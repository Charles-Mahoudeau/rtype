/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#include "World.hpp"

#include <cstddef>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/detail/WorldState.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/Resources.hpp"

namespace rtype::ecs {
Components& World::getComponents() noexcept { return _state->components; }

const Components& World::getComponents() const noexcept { return _state->components; }

Resources& World::getResources() noexcept { return _state->resources; }

const Resources& World::getResources() const noexcept { return _state->resources; }

Tick World::getChangeTick() const noexcept { return _state->changeTick; }

std::size_t World::getEntityCount() const noexcept { return _state->entities.getAliveCount(); }

Tick World::advanceChangeTick(Tick count) noexcept {
    _state->changeTick += count;  // wraps around after 2^32 ticks: comparisons and clamping are built for it
    if (_state->changeTick - _state->lastCheckTick >= kCheckTickThreshold) {
        checkChangeTicks();
    }
    return _state->changeTick;
}

void World::checkChangeTicks() noexcept {
    _state->components.clampTicks(_state->changeTick);
    _state->resources.clampTicks(_state->changeTick);
    _state->lastCheckTick = _state->changeTick;
}

Entity World::spawn() { return _state->entities.allocate(); }

bool World::despawn(Entity entity) noexcept {
    if (!isAlive(entity)) {
        return false;
    }
    _state->components.removeAll(entity);
    _state->components.clearReferencesTo(entity);
    _state->entities.release(entity);
    return true;
}

bool World::isAlive(Entity entity) const noexcept { return _state->entities.isAlive(entity); }
}  // namespace rtype::ecs
