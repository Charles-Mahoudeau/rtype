/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Scheduler
*/

#pragma once

#include <array>
#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/schedule/DependencyGraph.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief Runs a world's systems phase by phase: in the order their `before` / `after` constraints give, keeping the
/// systems of the process's run mode, and applying their commands after each phase.
///
/// @details Inside a phase, systems are sorted topologically; systems with no order between them run by name, so the
/// order never depends on the order systems were added. Building the schedule checks it and throws a
/// ScheduleException naming the systems involved for: two systems with the same name, a constraint naming an unknown
/// system or a system of another phase, and a cycle of constraints. Every system is checked whatever the run mode, so
/// a schedule that builds on the server also builds on clients. Execution is sequential; the systems' access sets are
/// kept for a parallel scheduler later. It keeps a pointer to the world: the world must outlive it.
/// @code
/// rtype::ecs::Scheduler scheduler{world, rtype::ecs::RunMode::kStandalone};
/// scheduler.addSystem(rtype::ecs::Phase::kFixedUpdate, "rtype.movement", &movement);
/// scheduler.addSystem(rtype::ecs::Phase::kFixedUpdate, "rtype.collision", &detect).after("rtype.movement");
/// scheduler.run(rtype::ecs::Phase::kFixedUpdate);
/// @endcode
class RTYPE_ECS_API Scheduler {
  public:
    /// @brief Configures a system just added: its order in its phase and its side, chained.
    class RTYPE_ECS_API SystemConfig {
      public:
        /// @param scheduler The scheduler the system was added to; it must outlive this configuration.
        /// @param index The system's position among the scheduler's systems.
        SystemConfig(Scheduler& scheduler, std::size_t index) noexcept;
        ~SystemConfig() = default;
        SystemConfig(const SystemConfig& other) noexcept = default;
        SystemConfig& operator=(const SystemConfig& other) noexcept = default;
        SystemConfig(SystemConfig&& other) noexcept = default;
        SystemConfig& operator=(SystemConfig&& other) noexcept = default;

        /// @brief Runs the system after the system named `name`, which must be in the same phase.
        SystemConfig& after(std::string name);

        /// @brief Runs the system before the system named `name`, which must be in the same phase.
        SystemConfig& before(std::string name);

        /// @brief Sets where the system runs (System::setSide()).
        SystemConfig& setSide(Side side) noexcept;

      private:
        Scheduler* _scheduler;  ///< The scheduler the system was added to.
        std::size_t _index;     ///< The system's position among the scheduler's systems.
    };

    /// @param world The world the systems run on; it must outlive the scheduler.
    /// @param mode What this process runs: it decides which systems run, from their side.
    Scheduler(World& world, RunMode mode) noexcept;
    ~Scheduler() = default;
    Scheduler(const Scheduler& other) = delete;
    Scheduler& operator=(const Scheduler& other) = delete;
    Scheduler(Scheduler&& other) noexcept = default;
    Scheduler& operator=(Scheduler&& other) noexcept = default;

    /// @return What this process runs.
    [[nodiscard]] RunMode getRunMode() const noexcept;

    /// @return The names of the phase's systems, in the order they run (every side, whatever the run mode), as of the
    /// last build().
    [[nodiscard]] std::vector<std::string_view> getOrder(Phase phase) const;

    /// @brief Makes a system of a function (or a lambda without captures) and adds it to a phase (System).
    template <typename Function>
    SystemConfig addSystem(Phase phase, std::string name, Function function);

    /// @brief Adds a system to a phase. The schedule is built again before the next run.
    SystemConfig addSystem(Phase phase, System system);

    /// @brief Checks the systems and sorts every phase.
    /// @throws exceptions::ScheduleException When two systems have the same name, a constraint names an unknown system
    /// or a system of another phase, or constraints form a cycle (the message lists its systems).
    void build();

    /// @brief Runs the phase's systems in order, skipping those whose side does not run in this mode, then applies
    /// the commands they recorded, in the same order. Builds the schedule first if it changed.
    /// @throws exceptions::ScheduleException When the schedule cannot be built.
    void run(Phase phase);

  private:
    /// @brief Each system's position in _entries, by name.
    using NameIndex = std::unordered_map<std::string_view, std::size_t>;

    /// @brief A system and where it goes.
    struct Entry {
        System system;                    ///< The system.
        Phase phase;                      ///< The phase it runs in.
        std::vector<std::string> after;   ///< Names of the systems it runs after.
        std::vector<std::string> before;  ///< Names of the systems it runs before.
    };

    /// @brief The systems of one phase, numbered from 0 for the dependency graph.
    struct PhaseSystems {
        std::vector<std::size_t> positions;   ///< Number -> position in _entries.
        std::vector<std::string_view> names;  ///< Number -> name.
        std::vector<std::size_t> numbers;     ///< Position in _entries -> number, or kNotInPhase.
    };

    /// @brief Marks a position in _entries whose system is not in the phase.
    static constexpr std::size_t kNotInPhase = std::numeric_limits<std::size_t>::max();

    /// @return Each system's position, by name.
    /// @throws exceptions::ScheduleException When two systems have the same name.
    [[nodiscard]] NameIndex indexNames() const;

    /// @return The positions of the phase's systems, in run order.
    /// @throws exceptions::ScheduleException When a constraint names an unknown system or one of another phase, or
    /// constraints form a cycle.
    [[nodiscard]] std::vector<std::size_t> sortPhase(Phase phase, const NameIndex& names) const;

    /// @return The phase's systems, numbered in the order they were added.
    [[nodiscard]] PhaseSystems collect(Phase phase) const;

    /// @return The phase's systems as a graph: one edge per `before` / `after` constraint.
    /// @throws exceptions::ScheduleException When a constraint names an unknown system or one of another phase.
    [[nodiscard]] detail::DependencyGraph makeGraph(Phase phase, const PhaseSystems& systems,
                                                    const NameIndex& names) const;

    /// @return The number in `systems` of the system named `target`, which `entry` runs before or after.
    /// @param relation "before" or "after", for the message.
    /// @throws exceptions::ScheduleException When no system has this name, or it is in another phase.
    [[nodiscard]] std::size_t resolve(Phase phase, const PhaseSystems& systems, const NameIndex& names,
                                      const Entry& entry, std::string_view relation, const std::string& target) const;

    World* _world;                                             ///< The world the systems run on.
    RunMode _mode;                                             ///< What this process runs.
    std::vector<Entry> _entries;                               ///< Every system, in the order added.
    std::array<std::vector<std::size_t>, kPhaseCount> _order;  ///< Per phase, positions in _entries, in run order.
    bool _built{false};                                        ///< Whether _order matches _entries.
};
}  // namespace rtype::ecs

#include "Scheduler.tpp"
