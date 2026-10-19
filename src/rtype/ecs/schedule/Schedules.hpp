/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Schedules
*/

#pragma once

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/schedule/FrameClock.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief A world's schedules, one per phase, and the frame that runs them.
///
/// @details runFrame() runs Startup on the first frame, then PreUpdate, FixedUpdate as many times as the frame clock
/// says (zero or more), Update, PostUpdate and Render, each schedule followed by its commands. It also moves the event
/// queues forward with the phase that writes them: a queue written by a FixedUpdate system after each fixed tick, so
/// its events are readable for two fixed ticks; every other queue at the end of the frame, for two frames. The clock
/// (FrameClock) is installed by the time plugin; without one, FixedUpdate never runs. System names are unique across
/// every schedule. It keeps a pointer to the world: the world must outlive it.
/// @code
/// rtype::ecs::Schedules schedules{world, rtype::ecs::RunMode::kStandalone};
/// schedules.addSystem(rtype::ecs::Phase::kFixedUpdate, "rtype.movement", &movement);
/// schedules.addSystem(rtype::ecs::Phase::kFixedUpdate, "rtype.collision", &detect).after("rtype.movement");
/// schedules.setClock(std::move(clock));
/// schedules.runFrame();
/// @endcode
class RTYPE_ECS_API Schedules {
  public:
    /// @param world The world the systems run on; it must outlive the schedules.
    /// @param mode What this process runs: it decides which systems run, from their side.
    Schedules(World& world, RunMode mode);
    ~Schedules() = default;
    Schedules(const Schedules& other) = delete;
    Schedules& operator=(const Schedules& other) = delete;
    Schedules(Schedules&& other) noexcept = default;
    Schedules& operator=(Schedules&& other) noexcept = default;

    /// @return What this process runs.
    [[nodiscard]] RunMode getRunMode() const noexcept;

    /// @return The schedule of a phase.
    [[nodiscard]] const Schedule& getSchedule(Phase phase) const noexcept;

    /// @return The names of the phase's systems, in the order they run, as of the last build of its schedule.
    [[nodiscard]] std::vector<std::string_view> getOrder(Phase phase) const;

    /// @return The frame clock, or nullptr if none is installed.
    [[nodiscard]] FrameClock* getClock() const noexcept;

    /// @brief Installs the frame clock, replacing the previous one.
    void setClock(std::unique_ptr<FrameClock> clock) noexcept;

    /// @brief Makes a system of a function (or a lambda without captures) and adds it to a phase (System).
    /// @param configs The configurations of its configured parameters (see SystemParam), if any.
    /// @throws exceptions::ScheduleException When a system of any phase already has this name.
    template <typename Function, typename... Configs>
    Schedule::SystemConfig addSystem(Phase phase, std::string name, Function function, Configs... configs);

    /// @brief Adds a system to a phase. Its schedule is built again before it next runs.
    /// @throws exceptions::ScheduleException When a system of any phase already has its name.
    Schedule::SystemConfig addSystem(Phase phase, System system);

    /// @brief Builds every schedule (Schedule::build()).
    /// @throws exceptions::ScheduleException When a schedule cannot be built.
    void build();

    /// @brief Runs one phase's schedule (Schedule::run()); neither the clock nor the event queues are touched.
    /// @throws exceptions::ScheduleException When the schedule cannot be built.
    void run(Phase phase);

    /// @brief Runs a whole frame: Startup on the first frame only, PreUpdate, FixedUpdate once per fixed tick the
    /// clock gives, Update, PostUpdate, then Render, each followed by its commands.
    /// @details Builds the schedules that changed first, so a bad schedule throws before the frame starts. Calls the
    /// clock's startFrame() first, finishFixedTick() after each FixedUpdate, startUpdate() before Update. Moves the
    /// queues of the events written in FixedUpdate forward after each fixed tick, and every other queue at the end of
    /// the frame (Events::update()).
    /// @throws exceptions::ScheduleException When a schedule cannot be built.
    void runFrame();

  private:
    /// @return The schedule of a phase.
    [[nodiscard]] Schedule& at(Phase phase) noexcept;

    /// @brief Builds every schedule that changed since its last build.
    void buildChanged();

    /// @brief Moves the queues of the events written in FixedUpdate forward: after each fixed tick.
    void updateFixedEvents() noexcept;

    /// @brief Moves every other queue forward: at the end of the frame.
    void updateFrameEvents() noexcept;

    World* _world;                                 ///< The world the systems run on.
    RunMode _mode;                                 ///< What this process runs.
    std::array<Schedule, kPhaseCount> _schedules;  ///< One schedule per phase, in phase order.
    std::unique_ptr<FrameClock> _clock;            ///< Decides the fixed ticks of each frame; may be null.
    bool _started{false};                          ///< Whether runFrame() has run Startup.
};
}  // namespace rtype::ecs

#include "Schedules.tpp"
