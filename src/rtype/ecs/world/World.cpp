/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#include "World.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <tl/expected.hpp>
#include <utility>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"
#include "rtype/ecs/storage/ResourceStorage.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
const ComponentRegistry& World::getRegistry() const noexcept { return _state->registry; }

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
    for (storage::SparseSetStorage& pool : _state->pools) {
        for (Tick& tick : pool.getAddedTicks()) {
            clampTick(tick, _state->changeTick);
        }
        for (Tick& tick : pool.getChangedTicks()) {
            clampTick(tick, _state->changeTick);
        }
    }
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
    // Every pool is checked: an O(1) lookup each. A per-entity list of its pools may replace the scan later.
    for (storage::SparseSetStorage& pool : _state->pools) {
        pool.remove(entity);
    }
    clearReferencesTo(entity);
    _state->entities.release(entity);
    return true;
}

bool World::isAlive(Entity entity) const noexcept { return _state->entities.isAlive(entity); }

Result<ComponentId> World::registerComponent(ComponentInfo info) {
    if (Result<void> free = checkNameFree(info.type.name, TypeKind::kComponent); !free) {
        return tl::unexpected<Error>{std::move(free.error())};
    }
    Result<ComponentId> id = _state->registry.registerType(std::move(info));
    if (id) {
        createPools(*id);
    }
    return id;
}

std::optional<ComponentId> World::findComponent(std::string_view name) const noexcept {
    if (const ComponentInfo* info = _state->registry.findByName(name)) {
        return info->type.id;
    }
    return std::nullopt;
}

bool World::add(Entity entity, ComponentId id, std::span<const std::byte> value) {
    storage::SparseSetStorage* pool = findPool(id);
    if (pool == nullptr || !isAlive(entity) || value.size() != pool->getInfo().type.size) {
        return false;
    }
    if (pool->contains(entity)) {
        pool->replaceCopy(entity, value, _state->changeTick);
    } else {
        pool->emplaceCopy(entity, value, _state->changeTick);
    }
    return true;
}

bool World::has(Entity entity, ComponentId id) const noexcept {
    const storage::SparseSetStorage* pool = findPool(id);
    return pool != nullptr && pool->contains(entity);
}

std::span<const std::byte> World::get(Entity entity, ComponentId id) const noexcept {
    const storage::SparseSetStorage* pool = findPool(id);
    return pool != nullptr ? pool->get(entity) : std::span<const std::byte>{};
}

std::span<std::byte> World::getMut(Entity entity, ComponentId id) noexcept {
    storage::SparseSetStorage* pool = findPool(id);
    if (pool == nullptr) {
        return {};
    }
    const std::optional<std::uint32_t> position = pool->find(entity);
    if (!position) {
        return {};
    }
    *std::next(pool->getChangedTicks().begin(), *position) = _state->changeTick;
    return pool->at(*position);
}

bool World::remove(Entity entity, ComponentId id) noexcept {
    storage::SparseSetStorage* pool = findPool(id);
    return pool != nullptr && pool->remove(entity);
}

std::optional<ComponentTicks> World::getTicks(Entity entity, ComponentId id) const noexcept {
    const storage::SparseSetStorage* pool = findPool(id);
    if (pool == nullptr) {
        return std::nullopt;
    }
    const std::optional<std::uint32_t> position = pool->find(entity);
    if (!position) {
        return std::nullopt;
    }
    return ComponentTicks{.added = *std::next(pool->getAddedTicks().begin(), *position),
                          .changed = *std::next(pool->getChangedTicks().begin(), *position)};
}

Result<void> World::checkNameFree(std::string_view name, TypeKind kind) const {
    const bool component = kind == TypeKind::kComponent;
    const bool taken =
        component ? _state->resourceRegistry.findByName(name) != nullptr : _state->registry.findByName(name) != nullptr;
    if (taken) {
        return fail(ErrorKind::kNameCollision,
                    std::format("{} '{}': the name is already used by a {}", component ? "Component" : "Resource", name,
                                component ? "resource" : "component"));
    }
    return {};
}

void World::createPools(ComponentId id) {
    // A loop, not a single push: if creating a pool ever threw, the next registration catches up.
    while (_state->pools.size() <= id) {
        _state->pools.emplace_back(*_state->registry.findById(static_cast<ComponentId>(_state->pools.size())));
    }
}

void World::clearReferencesTo(Entity entity) noexcept {
    for (storage::SparseSetStorage& pool : _state->pools) {
        for (const FieldInfo& field : pool.getInfo().type.fields) {
            if (field.type != FieldType::kEntity) {
                continue;
            }
            for (std::size_t position = 0; position < pool.getSize(); ++position) {
                const std::span<std::byte> value = pool.at(position);
                bool cleared = false;
                for (std::uint32_t element = 0; element < field.count; ++element) {
                    const std::span<std::byte> bytes =
                        value.subspan(field.offset + (element * sizeof(Entity)), sizeof(Entity));
                    Entity held;
                    std::memcpy(&held, bytes.data(), sizeof(Entity));
                    if (held == entity) {
                        const Entity null;
                        std::memcpy(bytes.data(), &null, sizeof(Entity));
                        cleared = true;
                    }
                }
                if (cleared) {
                    *std::next(pool.getChangedTicks().begin(), static_cast<std::ptrdiff_t>(position)) =
                        _state->changeTick;
                }
            }
        }
    }
}

storage::SparseSetStorage* World::findPool(ComponentId id) noexcept {
    return id < _state->pools.size() ? &*std::next(_state->pools.begin(), id) : nullptr;
}

const storage::SparseSetStorage* World::findPool(ComponentId id) const noexcept {
    return id < _state->pools.size() ? &*std::next(_state->pools.begin(), id) : nullptr;
}

Result<ResourceId> World::registerResource(ResourceInfo info) {
    if (Result<void> free = checkNameFree(info.type.name, TypeKind::kResource); !free) {
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
