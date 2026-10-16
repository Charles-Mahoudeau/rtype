/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#include "World.hpp"

#include <cstddef>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"
#include "rtype/ecs/storage/ResourceStorage.hpp"

namespace rtype::ecs {
Components& World::getComponents() noexcept { return _state->components; }

const Components& World::getComponents() const noexcept { return _state->components; }

const ResourceRegistry& World::getResourceRegistry() const noexcept { return _state->resourceRegistry; }

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
    for (storage::ResourceStorage& resource : _state->resources) {
        resource.clampTicks(_state->changeTick);
    }
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

Result<void> World::checkNameFree(std::string_view name) const {
    if (_state->components.getRegistry().findByName(name) != nullptr) {
        return fail(ErrorKind::kNameCollision,
                    std::format("Resource '{}': the name is already used by a component", name));
    }
    return {};
}

Result<ResourceId> World::registerResource(ResourceInfo info) {
    if (Result<void> free = checkNameFree(info.type.name); !free) {
        return tl::unexpected<Error>{std::move(free.error())};
    }
    Result<ResourceId> id = _state->resourceRegistry.registerType(std::move(info));
    if (id) {
        createResourceStorages(*id);
    }
    return id;
}

std::optional<ResourceId> World::findResource(std::string_view name) const noexcept {
    if (const ResourceInfo* info = _state->resourceRegistry.findByName(name)) {
        return info->type.id;
    }
    return std::nullopt;
}

bool World::insertResource(ResourceId id, std::span<const std::byte> value) {
    storage::ResourceStorage* resource = findResourceStorage(id);
    if (resource == nullptr || value.size() != resource->getInfo().type.size || !resource->getInfo().type.copyable) {
        return false;
    }
    resource->insertCopy(value, _state->changeTick);
    return true;
}

bool World::hasResource(ResourceId id) const noexcept {
    const storage::ResourceStorage* resource = findResourceStorage(id);
    return resource != nullptr && resource->hasValue();
}

std::span<const std::byte> World::getResource(ResourceId id) const noexcept {
    const storage::ResourceStorage* resource = findResourceStorage(id);
    return resource != nullptr ? resource->get() : std::span<const std::byte>{};
}

std::span<std::byte> World::getResourceMut(ResourceId id) noexcept {
    storage::ResourceStorage* resource = findResourceStorage(id);
    if (resource == nullptr || !resource->hasValue()) {
        return {};
    }
    resource->getTicks().changed = _state->changeTick;
    return resource->get();
}

bool World::removeResource(ResourceId id) noexcept {
    storage::ResourceStorage* resource = findResourceStorage(id);
    return resource != nullptr && resource->remove();
}

std::optional<ComponentTicks> World::getResourceTicks(ResourceId id) const noexcept {
    const storage::ResourceStorage* resource = findResourceStorage(id);
    if (resource == nullptr || !resource->hasValue()) {
        return std::nullopt;
    }
    return resource->getTicks();
}

void World::createResourceStorages(ResourceId id) {
    // A loop, not a single push: if creating a slot ever threw, the next registration catches up.
    while (_state->resources.size() <= id) {
        _state->resources.emplace_back(
            *_state->resourceRegistry.findById(static_cast<ResourceId>(_state->resources.size())));
    }
}

storage::ResourceStorage* World::findResourceStorage(ResourceId id) noexcept {
    return id < _state->resources.size() ? &*std::next(_state->resources.begin(), id) : nullptr;
}

const storage::ResourceStorage* World::findResourceStorage(ResourceId id) const noexcept {
    return id < _state->resources.size() ? &*std::next(_state->resources.begin(), id) : nullptr;
}
}  // namespace rtype::ecs
