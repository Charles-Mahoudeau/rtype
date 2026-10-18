/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Scheduler
*/

#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/schedule/DependencyGraph.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/Time.hpp"
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
///
/// runFrame() runs a whole frame: Startup on the first one, then PreUpdate, FixedUpdate as many times as the fixed
/// timestep gives (zero or more), Update, PostUpdate and Render, keeping the Time resource up to date. It also moves
/// the event queues forward with the phase that writes them: a queue written by a FixedUpdate system after each fixed
/// tick, so its events are readable for two fixed ticks; every other queue at the end of the frame, for two frames.
/// @code
/// rtype::ecs::Scheduler scheduler{world, rtype::ecs::RunMode::kStandalone};
/// scheduler.addSystem(rtype::ecs::Phase::kFixedUpdate, "rtype.movement", &movement);
/// scheduler.addSystem(rtype::ecs::Phase::kFixedUpdate, "rtype.collision", &detect).after("rtype.movement");
/// scheduler.runFrame(std::chrono::milliseconds{16});
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

    /// @return The fixed timestep runFrame() runs FixedUpdate with (60 ticks per second, at most 5 per frame, by
    /// default).
    [[nodiscard]] const FixedTimestep& getFixedTimestep() const noexcept;

    /// @return The names of the phase's systems, in the order they run (every side, whatever the run mode), as of the
    /// last build().
    [[nodiscard]] std::vector<std::string_view> getOrder(Phase phase) const;

    /// @brief Sets the fixed rate and the most fixed ticks per frame; the time not run yet is dropped.
    void setFixedTimestep(FixedTimestep timestep) noexcept;

    /// @brief Makes a system of a function (or a lambda without captures) and adds it to a phase (System).
    template <typename Function>
    SystemConfig addSystem(Phase phase, std::string name, Function function);

    /// @brief Adds a system to a phase. The schedule is built again before the next run.
    SystemConfig addSystem(Phase phase, System system);

    /// @brief Checks the systems, sorts every phase and finds the events written in FixedUpdate.
    /// @throws exceptions::ScheduleException When two systems have the same name, a constraint names an unknown system
    /// or a system of another phase, or constraints form a cycle (the message lists its systems).
    void build();

    /// @brief Runs the phase's systems in order, skipping those whose side does not run in this mode, then applies
    /// the commands they recorded, in the same order. Builds the schedule first if it changed.
    /// @throws exceptions::ScheduleException When the schedule cannot be built.
    void run(Phase phase);

    /// @brief Runs a whole frame: Startup on the first frame only, PreUpdate, FixedUpdate once per fixed tick the
    /// frame completes (FixedTimestep::advance()), Update, PostUpdate, then Render, each followed by its commands.
    /// @details Inserts the Time resource if it has no value, then keeps it up to date: `delta` and `fixedDelta`
    /// before PreUpdate, `tick` after each FixedUpdate (so while FixedUpdate runs it is the current tick), `alpha`
    /// before Update. Moves the queues of the events written by FixedUpdate systems of this run mode forward after
    /// each fixed tick, and every other queue at the end of the frame (Events::update()).
    /// @param frameDelta The time since the last frame, measured by the caller; a negative one counts as 0.
    /// @throws exceptions::ScheduleException When the schedule cannot be built.
    void runFrame(std::chrono::nanoseconds frameDelta);

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
    /// @throws exceptions::ScheduleException When a constraint names an unknown system or one of another phase,
    /// constraints form a cycle, or two systems touch the same data with no order between them.
    [[nodiscard]] std::vector<std::size_t> sortPhase(Phase phase, const NameIndex& names) const;

    /// @return The phase's systems, numbered in the order they were added.
    [[nodiscard]] PhaseSystems collect(Phase phase) const;

    /// @return The phase's systems as a graph: one edge per `before` / `after` constraint.
    /// @throws exceptions::ScheduleException When a constraint names an unknown system or one of another phase.
    [[nodiscard]] detail::DependencyGraph makeGraph(Phase phase, const PhaseSystems& systems,
                                                    const NameIndex& names) const;

    /// @brief Checks that every two systems of the phase touching the same data, one of them writing it, are ordered
    /// (one reaches the other in `graph`): otherwise their result would depend on the order they were added.
    /// @throws exceptions::ScheduleException Listing every such pair and the component, resource or event type.
    void checkAmbiguities(Phase phase, const PhaseSystems& systems, const detail::DependencyGraph& graph) const;

    /// @return One line per pair of the phase's systems that touch the same data, one of them writing it, with no order
    /// between them ("'a' and 'b' both use component 'x', one of them writing it"), the pairs in name order.
    [[nodiscard]] std::vector<std::string> findAmbiguities(const PhaseSystems& systems,
                                                           const detail::DependencyGraph& graph) const;

    /// @return The name of the first type `first` and `second` both use, one of them writing it, as
    /// "component 'rtype.Position'"; nothing if they do not conflict.
    [[nodiscard]] std::optional<std::string> findConflict(const System& first, const System& second) const;

    /// @return The number in `systems` of the system named `target`, which `entry` runs before or after.
    /// @param relation "before" or "after", for the message.
    /// @throws exceptions::ScheduleException When no system has this name, or it is in another phase.
    [[nodiscard]] std::size_t resolve(Phase phase, const PhaseSystems& systems, const NameIndex& names,
                                      const Entry& entry, std::string_view relation, const std::string& target) const;

    /// @return The events written by the FixedUpdate systems that run in this mode, sorted, without duplicates.
    [[nodiscard]] std::vector<EventId> collectFixedEvents() const;

    /// @brief Moves the queues of the events written in FixedUpdate forward: after each fixed tick.
    void updateFixedEvents() noexcept;

    /// @brief Moves every other queue forward: at the end of the frame.
    void updateFrameEvents() noexcept;

    /// @return Write access to the Time resource, inserted first if it has no value.
    [[nodiscard]] Mut<Time> getTime();

    World* _world;                                             ///< The world the systems run on.
    RunMode _mode;                                             ///< What this process runs.
    FixedTimestep _timestep;                                   ///< How many times each frame runs FixedUpdate.
    std::vector<Entry> _entries;                               ///< Every system, in the order added.
    std::array<std::vector<std::size_t>, kPhaseCount> _order;  ///< Per phase, positions in _entries, in run order.
    std::vector<EventId> _fixedEvents;  ///< Events written in FixedUpdate, sorted, as of the last build().
    bool _built{false};                 ///< Whether _order matches _entries.
    bool _started{false};               ///< Whether runFrame() has run Startup.
};
}  // namespace rtype::ecs

#include "Scheduler.tpp"
