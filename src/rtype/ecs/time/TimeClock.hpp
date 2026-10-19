/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TimeClock
*/

#pragma once

#include <chrono>
#include <cstdint>
#include <optional>

#include "rtype/ecs/Export.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/schedule/FrameClock.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
/// @brief The frame clock of the time plugin: measures each frame, runs the fixed timestep on it and keeps the Time
/// resource up to date.
/// @details Frames are measured with std::chrono::steady_clock (the first one counts 0), or all last a fixed manual
/// duration, for deterministic tests and replays. It inserts Time if it has no value, then sets `delta` and
/// `fixedDelta` at the start of the frame, `tick` after each FixedUpdate (so while FixedUpdate runs it is the current
/// tick), and `alpha` before Update.
/// @code
/// schedules.setClock(std::make_unique<rtype::ecs::TimeClock>(rtype::ecs::FixedTimestep{30, 5}));
/// schedules.setClock(std::make_unique<rtype::ecs::TimeClock>(rtype::ecs::FixedTimestep{},
/// std::chrono::milliseconds{16}));
/// @endcode
class RTYPE_ECS_API TimeClock : public FrameClock {
  public:
    /// @param timestep The fixed rate and the most fixed ticks per frame.
    /// @param manualFrame The duration of every frame; nothing to measure the real clock.
    explicit TimeClock(FixedTimestep timestep = FixedTimestep{},
                       std::optional<std::chrono::nanoseconds> manualFrame = std::nullopt) noexcept;
    ~TimeClock() override = default;
    TimeClock(const TimeClock& other) = delete;
    TimeClock& operator=(const TimeClock& other) = delete;
    TimeClock(TimeClock&& other) = delete;
    TimeClock& operator=(TimeClock&& other) = delete;

    /// @return The fixed timestep the frames run.
    [[nodiscard]] const FixedTimestep& getFixedTimestep() const noexcept;

    /// @return The duration of every frame, or nothing when the real clock is measured.
    [[nodiscard]] std::optional<std::chrono::nanoseconds> getManualFrame() const noexcept;

    /// @brief Measures the frame, sets Time::delta and Time::fixedDelta.
    /// @return The fixed ticks the frame completes (FixedTimestep::advance()).
    [[nodiscard]] std::uint32_t startFrame(World& world) override;

    /// @brief Counts the tick: Time::tick + 1.
    void finishFixedTick(World& world) override;

    /// @brief Sets Time::alpha, how far the frame is into the next tick.
    void startUpdate(World& world) override;

  private:
    /// @return The duration of the frame starting now: the manual one, or the time since the last frame started (0
    /// for the first).
    [[nodiscard]] std::chrono::nanoseconds measureFrame() noexcept;

    /// @return Write access to the Time resource, inserted first if it has no value.
    [[nodiscard]] Mut<Time> getTime(World& world) const;

    FixedTimestep _timestep;                                               ///< Turns frames into fixed ticks.
    std::optional<std::chrono::nanoseconds> _manualFrame;                  ///< Every frame's duration, if manual.
    std::optional<std::chrono::steady_clock::time_point> _lastFrameStart;  ///< When the last frame started.
};
}  // namespace rtype::ecs
