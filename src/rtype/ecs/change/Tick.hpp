/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Tick
*/

#pragma once

#include <cstdint>
#include <limits>

namespace rtype::ecs {
/// @brief A moment of the world, counted in system runs.
///
/// @details Pools stamp each value with the tick it was added at and the tick it was last written at; a system
/// compares those ticks with the tick of its own last run to know what was added or changed since.
using Tick = std::uint32_t;

/// @brief Number of ticks between two clampings of the stored ticks (about 6 days at 1000 system runs per second).
inline constexpr Tick kCheckTickThreshold = 518'400'000;

/// @brief Oldest age a stored tick may have: older ticks are clamped to it. It leaves room for two check periods,
/// so a tick checked late is still clamped before its age wraps around.
inline constexpr Tick kMaxChangeAge = std::numeric_limits<Tick>::max() - ((2 * kCheckTickThreshold) - 1);

/// @brief Tells whether `tick` is more recent than `lastRun`, both seen from `thisRun`.
/// @details Compares ages (`thisRun - tick` and `thisRun - lastRun`) in unsigned arithmetic, which wraps, so the
/// answer stays right after the 32-bit counter wraps around, as long as no tick is older than kMaxChangeAge.
/// @return True when `tick` happened after `lastRun`.
[[nodiscard]] constexpr bool isNewerThan(Tick tick, Tick lastRun, Tick thisRun) noexcept {
    const Tick tickAge = thisRun - tick;
    const Tick lastRunAge = thisRun - lastRun;
    return tickAge < lastRunAge;
}

/// @brief Moves `tick` forward so that, seen from `thisRun`, it is at most kMaxChangeAge old.
/// @details A clamped tick still compares as older than any recent tick: only the very old ones move.
constexpr void clampTick(Tick& tick, Tick thisRun) noexcept {
    if (thisRun - tick > kMaxChangeAge) {
        tick = thisRun - kMaxChangeAge;
    }
}
}  // namespace rtype::ecs
