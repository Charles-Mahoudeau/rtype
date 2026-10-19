/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TimeClock
*/

#include "TimeClock.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>

#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
TimeClock::TimeClock(FixedTimestep timestep, std::optional<std::chrono::nanoseconds> manualFrame) noexcept
    : _timestep{timestep}, _manualFrame{manualFrame} {}

const FixedTimestep& TimeClock::getFixedTimestep() const noexcept { return _timestep; }

std::optional<std::chrono::nanoseconds> TimeClock::getManualFrame() const noexcept { return _manualFrame; }

std::uint32_t TimeClock::startFrame(World& world) {
    const std::chrono::nanoseconds frame = measureFrame();
    Mut<Time> time = getTime(world);
    time->delta = std::chrono::duration<float>{std::max(frame, std::chrono::nanoseconds::zero())}.count();
    time->fixedDelta = _timestep.getTickSeconds();
    return _timestep.advance(frame);
}

void TimeClock::finishFixedTick(World& world) { ++getTime(world)->tick; }

void TimeClock::startUpdate(World& world) { getTime(world)->alpha = _timestep.getAlpha(); }

std::chrono::nanoseconds TimeClock::measureFrame() noexcept {
    if (_manualFrame) {
        return *_manualFrame;
    }
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    const std::chrono::nanoseconds frame =
        _lastFrameStart ? std::chrono::duration_cast<std::chrono::nanoseconds>(now - *_lastFrameStart)
                        : std::chrono::nanoseconds::zero();
    _lastFrameStart = now;
    return frame;
}

Mut<Time> TimeClock::getTime(World& world) const {
    Resources& resources = world.getResources();
    if (!resources.has<Time>()) {  // got each time: a system may have removed or replaced it
        resources.insert(Time{.delta = 0.F, .fixedDelta = _timestep.getTickSeconds(), .tick = 0, .alpha = 0.F});
    }
    return resources.getMut<Time>();
}
}  // namespace rtype::ecs
