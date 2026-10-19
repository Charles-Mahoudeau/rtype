/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Schedules
*/

#include "Schedules.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <iterator>
#include <memory>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/schedule/FrameClock.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
namespace {
/// @return One schedule per phase, in phase order.
template <std::size_t... Indices>
[[nodiscard]] std::array<Schedule, kPhaseCount> makeSchedules(World& world, RunMode mode,
                                                              std::index_sequence<Indices...> /*indices*/) {
    return {Schedule{world, mode, static_cast<Phase>(Indices)}...};
}
}  // namespace

Schedules::Schedules(World& world, RunMode mode)
    : _world{&world}, _mode{mode}, _schedules{makeSchedules(world, mode, std::make_index_sequence<kPhaseCount>{})} {}

RunMode Schedules::getRunMode() const noexcept { return _mode; }

const Schedule& Schedules::getSchedule(Phase phase) const noexcept {
    return *std::next(_schedules.begin(), static_cast<std::ptrdiff_t>(phase));
}

std::vector<std::string_view> Schedules::getOrder(Phase phase) const { return getSchedule(phase).getOrder(); }

FrameClock* Schedules::getClock() const noexcept { return _clock.get(); }

void Schedules::setClock(std::unique_ptr<FrameClock> clock) noexcept { _clock = std::move(clock); }

Schedule::SystemConfig Schedules::addSystem(Phase phase, System system) {
    const Schedule& target = getSchedule(phase);  // its own duplicates: reported by Schedule::addSystem()
    for (const Schedule& schedule : _schedules) {
        if (&schedule != &target && schedule.contains(system.getName())) {
            throw exceptions::ScheduleException{
                std::format("Two systems are named '{}' (phases {} and {}): names must be unique", system.getName(),
                            toString(schedule.getPhase()), toString(phase))};
        }
    }
    return at(phase).addSystem(std::move(system));
}

void Schedules::build() {
    for (Schedule& schedule : _schedules) {
        schedule.build();
    }
}

void Schedules::run(Phase phase) { at(phase).run(); }

void Schedules::runFrame() {
    buildChanged();
    const std::uint32_t ticks = _clock ? _clock->startFrame(*_world) : 0;
    if (!_started) {
        run(Phase::kStartup);
        _started = true;
    }
    run(Phase::kPreUpdate);
    for (std::uint32_t done = 0; done < ticks; ++done) {
        run(Phase::kFixedUpdate);
        updateFixedEvents();
        if (_clock) {
            _clock->finishFixedTick(*_world);
        }
    }
    if (_clock) {
        _clock->startUpdate(*_world);
    }
    run(Phase::kUpdate);
    run(Phase::kPostUpdate);
    run(Phase::kRender);
    updateFrameEvents();
}

Schedule& Schedules::at(Phase phase) noexcept {
    return *std::next(_schedules.begin(), static_cast<std::ptrdiff_t>(phase));
}

void Schedules::buildChanged() {
    for (Schedule& schedule : _schedules) {
        if (!schedule.isBuilt()) {
            schedule.build();
        }
    }
}

void Schedules::updateFixedEvents() noexcept {
    for (const EventId id : getSchedule(Phase::kFixedUpdate).getWrittenEvents()) {
        static_cast<void>(_world->getEvents().update(id));
    }
}

void Schedules::updateFrameEvents() noexcept {
    Events& events = _world->getEvents();
    const std::span<const EventId> fixed = getSchedule(Phase::kFixedUpdate).getWrittenEvents();
    const std::size_t count = events.getRegistry().getCount();  // ids are 0 to count - 1
    for (std::size_t index = 0; index < count; ++index) {
        const auto id = static_cast<EventId>(index);
        if (!std::ranges::binary_search(fixed, id)) {
            static_cast<void>(events.update(id));
        }
    }
}
}  // namespace rtype::ecs
