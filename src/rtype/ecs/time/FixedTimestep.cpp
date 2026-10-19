/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FixedTimestep
*/

#include "FixedTimestep.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdint>

namespace rtype::ecs {
namespace {
/// @brief Accumulator units in one tick: a tick lasts 1/rate s, i.e. 10^9 / rate ns, i.e. 10^9 units.
constexpr std::int64_t kUnitsPerTick = 1'000'000'000;

/// @brief The longest frame counted: keeps nanoseconds times ticks per second far from overflowing.
constexpr std::chrono::nanoseconds kLongestFrame = std::chrono::hours{1};
}  // namespace

FixedTimestep::FixedTimestep(std::uint32_t ticksPerSecond, std::uint32_t maxTicksPerFrame) noexcept
    : _rate{ticksPerSecond}, _maxTicks{maxTicksPerFrame} {
    assert(ticksPerSecond > 0 && maxTicksPerFrame > 0);
}

std::uint32_t FixedTimestep::getRate() const noexcept { return _rate; }

std::uint32_t FixedTimestep::getMaxTicksPerFrame() const noexcept { return _maxTicks; }

float FixedTimestep::getTickSeconds() const noexcept { return 1.F / static_cast<float>(_rate); }

float FixedTimestep::getAlpha() const noexcept {
    return static_cast<float>(static_cast<double>(_accumulated) / static_cast<double>(kUnitsPerTick));
}

std::uint32_t FixedTimestep::advance(std::chrono::nanoseconds frameDelta) noexcept {
    const std::chrono::nanoseconds counted = std::clamp(frameDelta, std::chrono::nanoseconds::zero(), kLongestFrame);
    _accumulated += counted.count() * static_cast<std::int64_t>(_rate);
    const std::int64_t ticks = _accumulated / kUnitsPerTick;
    if (ticks >= _maxTicks) {
        _accumulated %= kUnitsPerTick;  // the cap: the time beyond it is dropped, only the start of a tick is kept
        return _maxTicks;
    }
    _accumulated -= ticks * kUnitsPerTick;
    return static_cast<std::uint32_t>(ticks);
}
}  // namespace rtype::ecs
