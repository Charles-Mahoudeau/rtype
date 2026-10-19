/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FixedTimestep
*/

#pragma once

#include <chrono>
#include <cstdint>

#include "rtype/ecs/Export.hpp"

namespace rtype::ecs {
/// @brief Turns frame durations into a whole number of fixed ticks: the accumulator that runs FixedUpdate zero or
/// more times per frame at a fixed rate.
///
/// @details Time is counted in integers (nanoseconds times ticks per second, one tick being exactly 10^9 units), so
/// every machine counts the same ticks for the same frames, and 60 Hz gives exactly 60 ticks per second: no float
/// drift and no rounding of 1/60 s. A frame may give no tick (a fast frame) or several (a slow one), up to a cap:
/// beyond it the extra time is dropped, so on a machine that cannot keep up the game slows down instead of spending
/// ever longer catching up (the "spiral of death").
/// @code
/// rtype::ecs::FixedTimestep timestep{60, 5};
/// const std::uint32_t ticks = timestep.advance(std::chrono::milliseconds{50});  // 3
/// @endcode
class RTYPE_ECS_API FixedTimestep {
  public:
    static constexpr std::uint32_t kDefaultRate = 60;     ///< Fixed ticks per second by default.
    static constexpr std::uint32_t kDefaultMaxTicks = 5;  ///< Most fixed ticks one frame may run, by default.

    /// @param ticksPerSecond The fixed rate; must be above 0.
    /// @param maxTicksPerFrame The most ticks one frame may run; must be above 0.
    explicit FixedTimestep(std::uint32_t ticksPerSecond = kDefaultRate,
                           std::uint32_t maxTicksPerFrame = kDefaultMaxTicks) noexcept;
    ~FixedTimestep() = default;
    FixedTimestep(const FixedTimestep& other) noexcept = default;
    FixedTimestep& operator=(const FixedTimestep& other) noexcept = default;
    FixedTimestep(FixedTimestep&& other) noexcept = default;
    FixedTimestep& operator=(FixedTimestep&& other) noexcept = default;

    /// @return The fixed rate, in ticks per second.
    [[nodiscard]] std::uint32_t getRate() const noexcept;

    /// @return The most ticks one frame may run.
    [[nodiscard]] std::uint32_t getMaxTicksPerFrame() const noexcept;

    /// @return The duration of one tick, in seconds (Time::fixedDelta).
    [[nodiscard]] float getTickSeconds() const noexcept;

    /// @return How far the accumulated time is into the next tick, in [0, 1) (Time::alpha).
    [[nodiscard]] float getAlpha() const noexcept;

    /// @brief Adds a frame's duration and takes the whole ticks it completes.
    /// @param frameDelta The frame's duration; a negative one counts as 0, one above an hour as an hour.
    /// @return The ticks to run now, at most the cap; when the cap is reached, the time beyond it is dropped.
    std::uint32_t advance(std::chrono::nanoseconds frameDelta) noexcept;

  private:
    std::uint32_t _rate;           ///< Ticks per second.
    std::uint32_t _maxTicks;       ///< Most ticks per frame.
    std::int64_t _accumulated{0};  ///< Time not run yet, in nanoseconds times ticks per second (10^9 per tick).
};
}  // namespace rtype::ecs
