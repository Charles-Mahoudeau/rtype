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
#include <iterator>
#include <optional>
#include <span>
#include <string_view>
#include <utility>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"
#include "rtype/ecs/type/Field.hpp"

namespace rtype::ecs {
const ComponentRegistry& World::getRegistry() const noexcept { return _registry; }

Tick World::getChangeTick() const noexcept { return _changeTick; }

std::size_t World::getEntityCount() const noexcept { return _entities.getAliveCount(); }

Tick World::advanceChangeTick(Tick count) noexcept {
    _changeTick += count;  // wraps around after 2^32 ticks: comparisons and clamping are built for it
    if (_changeTick - _lastCheckTick >= kCheckTickThreshold) {
        checkChangeTicks();
    }
    return _changeTick;
}

void World::checkChangeTicks() noexcept {
    for (storage::SparseSetStorage& pool : _pools) {
        for (Tick& tick : pool.getAddedTicks()) {
            clampTick(tick, _changeTick);
        }
        for (Tick& tick : pool.getChangedTicks()) {
            clampTick(tick, _changeTick);
        }
    }
    _lastCheckTick = _changeTick;
}

Entity World::spawn() { return _entities.allocate(); }

bool World::despawn(Entity entity) noexcept {
    if (!isAlive(entity)) {
        return false;
    }
    // Every pool is checked: an O(1) lookup each. A per-entity list of its pools may replace the scan later.
    for (storage::SparseSetStorage& pool : _pools) {
        pool.remove(entity);
    }
    clearReferencesTo(entity);
    _entities.release(entity);
    return true;
}

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
        pool->replaceCopy(entity, value, _changeTick);
    } else {
        pool->emplaceCopy(entity, value, _changeTick);
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
    *std::next(pool->getChangedTicks().begin(), *position) = _changeTick;
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

void World::createPools(ComponentId id) {
    // A loop, not a single push: if creating a pool ever threw, the next registration catches up.
    while (_pools.size() <= id) {
        _pools.emplace_back(*_registry.findById(static_cast<ComponentId>(_pools.size())));
    }
}

void World::clearReferencesTo(Entity entity) noexcept {
    for (storage::SparseSetStorage& pool : _pools) {
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
                    *std::next(pool.getChangedTicks().begin(), static_cast<std::ptrdiff_t>(position)) = _changeTick;
                }
            }
        }
    }
}

storage::SparseSetStorage* World::findPool(ComponentId id) noexcept {
    return id < _pools.size() ? &*std::next(_pools.begin(), id) : nullptr;
}

const storage::SparseSetStorage* World::findPool(ComponentId id) const noexcept {
    return id < _pools.size() ? &*std::next(_pools.begin(), id) : nullptr;
}
}  // namespace rtype::ecs
