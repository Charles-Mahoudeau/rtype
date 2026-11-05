/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicEvents
*/

#include "DynamicEvents.hpp"

#include <cassert>
#include <cstddef>
#include <format>
#include <iterator>
#include <span>
#include <utility>

#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventRange.hpp"
#include "rtype/ecs/event/Events.hpp"
#include "rtype/ecs/exceptions/TypeExceptions.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/system/SystemParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::ecs {
DynamicEvents::DynamicEvents(Events& events, std::span<const EventUse> uses, std::span<EventCursor> cursors) noexcept
    : _events{&events}, _uses{uses}, _cursors{cursors} {
    assert(uses.size() == cursors.size());
}

std::size_t DynamicEvents::getCount() const noexcept { return _uses.size(); }

const EventUse& DynamicEvents::getUse(const std::size_t index) const noexcept {
    assert(index < _uses.size());
    return *std::next(_uses.begin(), static_cast<std::ptrdiff_t>(index));
}

RawEventRange DynamicEvents::read(const std::size_t index) const noexcept {
    const EventUse& use = getUse(index);
    if (!use.read) {
        return {};
    }
    return _events->read(use.id, *std::next(_cursors.begin(), static_cast<std::ptrdiff_t>(index)));
}

bool DynamicEvents::send(const std::size_t index, const std::span<const std::byte> value) const {
    const EventUse& use = getUse(index);
    return use.send && _events->send(use.id, value);
}

SystemParam<DynamicEvents>::State SystemParam<DynamicEvents>::init(World& world, Config config) {
    for (const EventUse& use : config.events) {
        if (world.getEvents().getRegistry().findById(use.id) == nullptr) {
            throw exceptions::TypeRegistrationException{
                std::format("An event of the system (id {}) is not registered", use.id)};
        }
    }
    State state;
    state.cursors.resize(config.events.size());
    state.uses = std::move(config.events);
    return state;
}

DynamicEvents SystemParam<DynamicEvents>::get(State& state, const SystemContext& context) noexcept {
    return DynamicEvents{context.world->getEvents(), state.uses, state.cursors};
}

void SystemParam<DynamicEvents>::addAccess(const State& state, SystemAccess& access) {
    for (const EventUse& use : state.uses) {
        access.addEvent(use.id, use.send);
    }
}
}  // namespace rtype::ecs
