/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Schedule
*/

#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/schedule/DependencyGraph.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief The systems of one phase: run in the order their `before` / `after` constraints give, keeping the systems
/// of the process's run mode, then their commands applied.
///
/// @details Systems are sorted topologically; systems with no order between them run by name, so the order never
/// depends on the order systems were added. Building the schedule checks it and throws a ScheduleException naming
/// the systems involved for: a constraint naming a system that is not in the schedule, a cycle of constraints, and two
/// systems touching the same data, one of them writing it, with no order between them. Every system is checked
/// whatever the run mode, so a schedule that builds on the server also builds on clients. Execution is sequential;
/// the systems' access sets are kept for a parallel scheduler later. It keeps a pointer to the world: the world must
/// outlive it. A world's schedules, one per phase, are held by Schedules.
/// @code
/// rtype::ecs::Schedule schedule{world, rtype::ecs::RunMode::kStandalone, rtype::ecs::Phase::kFixedUpdate};
/// schedule.addSystem("rtype.movement", &movement);
/// schedule.addSystem("rtype.collision", &detect).after("rtype.movement");
/// schedule.run();
/// @endcode
class RTYPE_ECS_API Schedule {
  public:
    /// @brief Configures a system just added: its order in the schedule and its side, chained.
    class RTYPE_ECS_API SystemConfig {
      public:
        /// @param schedule The schedule the system was added to; it must outlive this configuration.
        /// @param index The system's position among the schedule's systems.
        SystemConfig(Schedule& schedule, std::size_t index) noexcept;
        ~SystemConfig() = default;
        SystemConfig(const SystemConfig& other) noexcept = default;
        SystemConfig& operator=(const SystemConfig& other) noexcept = default;
        SystemConfig(SystemConfig&& other) noexcept = default;
        SystemConfig& operator=(SystemConfig&& other) noexcept = default;

        /// @brief Runs the system after the system named `name`, which must be in the same schedule.
        SystemConfig& after(std::string name);

        /// @brief Runs the system before the system named `name`, which must be in the same schedule.
        SystemConfig& before(std::string name);

        /// @brief Sets where the system runs (System::setSide()).
        SystemConfig& setSide(Side side) noexcept;

      private:
        Schedule* _schedule;  ///< The schedule the system was added to.
        std::size_t _index;   ///< The system's position among the schedule's systems.
    };

    /// @param world The world the systems run on; it must outlive the schedule.
    /// @param mode What this process runs: it decides which systems run, from their side.
    /// @param phase The phase the schedule runs, for messages.
    Schedule(World& world, RunMode mode, Phase phase) noexcept;
    ~Schedule() = default;
    Schedule(const Schedule& other) = delete;
    Schedule& operator=(const Schedule& other) = delete;
    Schedule(Schedule&& other) noexcept = default;
    Schedule& operator=(Schedule&& other) noexcept = default;

    /// @return What this process runs.
    [[nodiscard]] RunMode getRunMode() const noexcept;

    /// @return The phase the schedule runs.
    [[nodiscard]] Phase getPhase() const noexcept;

    /// @return The names of the systems, in the order they run (every side, whatever the run mode), as of the last
    /// build().
    [[nodiscard]] std::vector<std::string_view> getOrder() const;

    /// @return The events written (EventWriter) by the systems that run in this mode, sorted, as of the last build().
    [[nodiscard]] std::span<const EventId> getWrittenEvents() const noexcept;

    /// @return Whether the last build() matches the systems and their constraints.
    [[nodiscard]] bool isBuilt() const noexcept;

    /// @return Whether a system of the schedule has this name.
    [[nodiscard]] bool contains(std::string_view name) const noexcept;

    /// @brief Makes a system of a function (or a lambda without captures) and adds it (System).
    /// @param configs The configurations of its configured parameters (see SystemParam), if any.
    /// @throws exceptions::ScheduleException When a system of the schedule already has this name.
    template <typename Function, typename... Configs>
    SystemConfig addSystem(std::string name, Function function, Configs... configs);

    /// @brief Adds a system. The schedule is built again before the next run.
    /// @throws exceptions::ScheduleException When a system of the schedule already has its name.
    SystemConfig addSystem(System system);

    /// @brief Checks the systems, sorts them and lists the events they write.
    /// @throws exceptions::ScheduleException When a constraint names a system that is not in the schedule,
    /// constraints form a cycle (the message lists its systems), or two systems touch the same data with no order
    /// between them (the message lists every such pair).
    void build();

    /// @brief Runs the systems in order, skipping those whose side does not run in this mode, then applies the
    /// commands they recorded, in the same order. Builds the schedule first if it changed.
    /// @throws exceptions::ScheduleException When the schedule cannot be built.
    void run();

  private:
    /// @brief Each system's position in _entries, by name.
    using NameIndex = std::unordered_map<std::string_view, std::size_t>;

    /// @brief A system and its constraints.
    struct Entry {
        System system;                    ///< The system.
        std::vector<std::string> after;   ///< Names of the systems it runs after.
        std::vector<std::string> before;  ///< Names of the systems it runs before.
    };

    /// @return The systems' names, by position.
    [[nodiscard]] std::vector<std::string_view> getNames() const;

    /// @return The systems as a graph: one edge per `before` / `after` constraint.
    /// @throws exceptions::ScheduleException When a constraint names a system that is not in the schedule.
    [[nodiscard]] detail::DependencyGraph makeGraph() const;

    /// @return The position of the system named `target`, which `entry` runs before or after.
    /// @param relation "before" or "after", for the message.
    /// @throws exceptions::ScheduleException When no system of the schedule has this name.
    [[nodiscard]] std::size_t resolve(const NameIndex& names, const Entry& entry, std::string_view relation,
                                      const std::string& target) const;

    /// @brief Checks that every two systems touching the same data, one of them writing it, are ordered (one reaches
    /// the other in `graph`): otherwise their result would depend on the order they were added.
    /// @throws exceptions::ScheduleException Listing every such pair and the component, resource or event type.
    void checkAmbiguities(const std::vector<std::string_view>& names, const detail::DependencyGraph& graph) const;

    /// @return One line per pair of systems that touch the same data, one of them writing it, with no order between
    /// them ("'a' and 'b' both use component 'x', one of them writing it"), the pairs in name order.
    [[nodiscard]] std::vector<std::string> findAmbiguities(const std::vector<std::string_view>& names,
                                                           const detail::DependencyGraph& graph) const;

    /// @return The name of the first type `first` and `second` both use, one of them writing it, as
    /// "component 'rtype.Position'"; nothing if they do not conflict.
    [[nodiscard]] std::optional<std::string> findConflict(const System& first, const System& second) const;

    /// @return The events written by the systems that run in this mode, sorted, without duplicates.
    [[nodiscard]] std::vector<EventId> collectWrittenEvents() const;

    World* _world;                        ///< The world the systems run on.
    RunMode _mode;                        ///< What this process runs.
    Phase _phase;                         ///< The phase the schedule runs.
    std::vector<Entry> _entries;          ///< Every system, in the order added.
    std::vector<std::size_t> _order;      ///< Positions in _entries, in run order.
    std::vector<EventId> _writtenEvents;  ///< Events written by the systems of this run mode, sorted.
    bool _built{false};                   ///< Whether _order and _writtenEvents match _entries.
};
}  // namespace rtype::ecs

#include "Schedule.tpp"
