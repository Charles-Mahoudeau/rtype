/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Schedule
*/

#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
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
/// @details A system can join named sets (SystemConfig::inSet()): a set is ordered as a whole (configureSet()), its
/// constraints applying to every member, so a plugin places a system in a stage of the phase without naming the other
/// systems there. Systems and sets share one namespace: a `before` / `after` names either. A set with no member, or
/// never configured, is valid, and an order through it still holds. Systems are sorted topologically; systems with no
/// order between them run by name, so the order never depends on the order systems were added. Building the schedule
/// checks it and throws a ScheduleException naming the systems involved for: a constraint naming neither a system nor
/// a set of the schedule, a set named like a system, a cycle of constraints, and two systems touching the same data,
/// one of them writing it, with no order between them, unless it is a resource whose writes commute
/// (allowAmbiguousResource()). Every system is checked whatever the run mode, so a schedule that builds on the server
/// also builds on clients. Execution is sequential; the systems' access sets are kept for a parallel scheduler later.
/// It keeps a pointer to the world: the world must outlive it. A world's schedules, one per phase, are held by
/// Schedules.
/// @code
/// rtype::ecs::Schedule schedule{world, rtype::ecs::RunMode::kStandalone, rtype::ecs::Phase::kFixedUpdate};
/// schedule.addSystem("rtype.movement", &movement);
/// schedule.addSystem("rtype.collision", &detect).after("rtype.movement");
/// schedule.configureSet("game.effects").after("rtype.collision");
/// schedule.addSystem("game.sparks", &sparks).inSet("game.effects");  // runs after the collision
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

        /// @brief Runs the system after the system or every member of the set named `name`, in the same schedule.
        SystemConfig& after(std::string name);

        /// @brief Runs the system before the system or every member of the set named `name`, in the same schedule.
        SystemConfig& before(std::string name);

        /// @brief Makes the system a member of the set named `set`, which it creates if needed: the set's constraints
        /// apply to it. A system can join several sets.
        SystemConfig& inSet(std::string set);

        /// @brief Sets where the system runs (System::setSide()).
        /// @throws exceptions::ScheduleException When the side is Side::kClient and the system writes a component
        /// that replication overwrites (any ReplicationMode but kNone) or that is Presence::kServerOnly. The side is
        /// left as it was.
        SystemConfig& setSide(Side side);

      private:
        Schedule* _schedule;  ///< The schedule the system was added to.
        std::size_t _index;   ///< The system's position among the schedule's systems.
    };

    /// @brief Configures a set: its order in the schedule as a whole, chained. Every member of the set follows it.
    class RTYPE_ECS_API SetConfig {
      public:
        /// @param schedule The schedule the set belongs to; it must outlive this configuration.
        /// @param name The set's name.
        SetConfig(Schedule& schedule, std::string name);
        ~SetConfig() = default;
        SetConfig(const SetConfig& other) = default;
        SetConfig& operator=(const SetConfig& other) = default;
        SetConfig(SetConfig&& other) noexcept = default;
        SetConfig& operator=(SetConfig&& other) noexcept = default;

        /// @brief Runs every member of the set after the system or every member of the set named `name`.
        SetConfig& after(std::string name);

        /// @brief Runs every member of the set before the system or every member of the set named `name`.
        SetConfig& before(std::string name);

      private:
        Schedule* _schedule;  ///< The schedule the set belongs to.
        std::string _name;    ///< The set's name.
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

    /// @return The system of the schedule with this name, or null if there is none; valid until a system is added.
    [[nodiscard]] const System* findSystem(std::string_view name) const noexcept;

    /// @return Whether the schedule has a set of this name: configured, or joined by a system.
    [[nodiscard]] bool containsSet(std::string_view name) const noexcept;

    /// @brief Makes a system of a function (or a lambda without captures) and adds it (System).
    /// @param configs The configurations of its configured parameters (see SystemParam), if any.
    /// @throws exceptions::ScheduleException When a system of the schedule already has this name.
    template <typename Function, typename... Configs>
    SystemConfig addSystem(std::string name, Function function, Configs... configs);

    /// @brief Adds a system. The schedule is built again before the next run.
    /// @throws exceptions::ScheduleException When a system of the schedule already has its name.
    SystemConfig addSystem(System system);

    /// @return Whether two systems may write the resource `id` with no order between them (allowAmbiguousResource()).
    [[nodiscard]] bool isAmbiguousResource(ResourceId id) const noexcept;

    /// @brief Lets systems write the resource `id` with no order between them: its writes commute, so their order
    /// does not change the result (a queue sorted before it is read). Two such systems still conflict on any other
    /// data. The schedule is built again before the next run.
    void allowAmbiguousResource(ResourceId id);

    /// @brief Creates the set named `name` if needed, to order it as a whole. The schedule is built again before the
    /// next run.
    SetConfig configureSet(std::string name);

    /// @brief Checks the systems, sorts them and lists the events they write.
    /// @throws exceptions::ScheduleException When a constraint names neither a system nor a set of the schedule, a set
    /// is named like a system, constraints form a cycle (the message lists its systems and sets), or two systems touch
    /// the same data with no order between them (the message lists every such pair).
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
        std::vector<std::string> after;   ///< Names of the systems and sets it runs after.
        std::vector<std::string> before;  ///< Names of the systems and sets it runs before.
        std::vector<std::string> sets;    ///< Names of the sets it belongs to.
    };

    /// @brief A set's constraints, which apply to each of its members.
    struct SetEntry {
        std::vector<std::string> after;   ///< Names of the systems and sets its members run after.
        std::vector<std::string> before;  ///< Names of the systems and sets its members run before.
    };

    /// @brief The systems and sets as one graph: the systems are its first nodes, in the order of _entries; then each
    /// set, in the order of _sets, is two nodes, its start and its end, around its members.
    struct Graph {
        detail::DependencyGraph graph;  ///< One edge per constraint and per membership.
        std::vector<std::string>
            names;  ///< Each node's name for the sort: a set's nodes cannot be named like a system.
        std::vector<std::string> shown;  ///< Each node as messages show it: `'name'`, or `set 'name'`.
    };

    /// @brief Checks that a system that would run on clients writes no replicated or server-only component.
    /// @throws exceptions::ScheduleException Naming the system and the first component it may not write.
    void checkClientWrites(const System& system) const;

    /// @return The systems' names, by position.
    [[nodiscard]] std::vector<std::string_view> getNames() const;

    /// @return The systems and the sets as a graph (Graph).
    /// @throws exceptions::ScheduleException When a constraint names neither a system nor a set of the schedule.
    [[nodiscard]] Graph makeGraph() const;

    /// @return The node a constraint on `target` attaches to: the system, or the set's start (to run before it) or end
    /// (to run after it).
    /// @param owner The system or set constrained, as messages show it (`System 'a'`).
    /// @param relation "before" or "after": which of a set's nodes to take, and the message.
    /// @throws exceptions::ScheduleException When `target` is neither a system nor a set of the schedule.
    [[nodiscard]] std::size_t resolve(const NameIndex& names, std::string_view owner, std::string_view relation,
                                      const std::string& target) const;

    /// @return The node of the start of the set named `name`, which must exist; its end is the next node.
    [[nodiscard]] std::size_t getSetStart(std::string_view name) const;

    /// @throws exceptions::ScheduleException When a set has the name of a system.
    void checkSetNames() const;

    /// @brief Checks that every two systems touching the same data, one of them writing it, are ordered (one reaches
    /// the other in `graph`): otherwise their result would depend on the order they were added.
    /// @throws exceptions::ScheduleException Listing every such pair and the component, resource or event type.
    void checkAmbiguities(const std::vector<std::string_view>& names, const detail::DependencyGraph& graph) const;

    /// @return One line per pair of systems that touch the same data, one of them writing it, with no order between
    /// them ("'a' and 'b' both use component 'x', one of them writing it"), the pairs in name order.
    [[nodiscard]] std::vector<std::string> findAmbiguities(const std::vector<std::string_view>& names,
                                                           const detail::DependencyGraph& graph) const;

    /// @return The name of the first type `first` and `second` both use, one of them writing it, as
    /// "component 'rtype.Position'"; nothing if they do not conflict. The ambiguous resources are not conflicts.
    [[nodiscard]] std::optional<std::string> findConflict(const System& first, const System& second) const;

    /// @return The events written by the systems that run in this mode, sorted, without duplicates.
    [[nodiscard]] std::vector<EventId> collectWrittenEvents() const;

    World* _world;                                       ///< The world the systems run on.
    RunMode _mode;                                       ///< What this process runs.
    Phase _phase;                                        ///< The phase the schedule runs.
    std::vector<Entry> _entries;                         ///< Every system, in the order added.
    std::map<std::string, SetEntry, std::less<>> _sets;  ///< Every set, configured or joined, by name.
    std::vector<ResourceId> _ambiguousResources;         ///< Resources whose writes commute.
    std::vector<std::size_t> _order;                     ///< Positions in _entries, in run order.
    std::vector<EventId> _writtenEvents;                 ///< Events written by the systems of this run mode, sorted.
    bool _built{false};                                  ///< Whether _order and _writtenEvents match _entries.
};
}  // namespace rtype::ecs

#include "Schedule.tpp"
