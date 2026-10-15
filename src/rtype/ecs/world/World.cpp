/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#include "World.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Result.hpp"

namespace rtype::ecs {
const ComponentRegistry& World::getRegistry() const noexcept { return _registry; }

Tick World::getChangeTick() const noexcept { return _changeTick; }

std::size_t World::getEntityCount() const noexcept { return _entities.getAliveCount(); }

Entity World::spawn() { return _entities.allocate(); }

bool World::isAlive(Entity entity) const noexcept { return _entities.isAlive(entity); }

Result<ComponentId> World::registerComponent(ComponentInfo info) {
    Result<ComponentId> id = _registry.registerComponent(std::move(info));
    if (id) {
        createPools(*id);
    }
    return id;
}

std::optional<ComponentId> World::findComponent(std::string_view name) const noexcept {
    if (const ComponentInfo* info = _registry.findByName(name)) {
        return info->id;
    }
    return std::nullopt;
}

void World::createPools(ComponentId id) {
    // A loop, not a single push: if creating a pool ever threw, the next registration catches up.
    while (_pools.size() <= id) {
        _pools.emplace_back(*_registry.findById(static_cast<ComponentId>(_pools.size())));
    }
}
}  // namespace rtype::ecs
