/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Components
*/

#include "Components.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
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
#include "rtype/ecs/detail/WorldState.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs {
Components::Components(const detail::WorldState& state) noexcept : _state{&state} {}

const ComponentRegistry& Components::getRegistry() const noexcept { return _registry; }

Result<ComponentId> Components::registerType(ComponentInfo info) {
    if (Result<void> free = checkNameFree(info.type.name); !free) {
        return tl::unexpected<Error>{std::move(free.error())};
    }
    Result<ComponentId> id = _registry.registerType(std::move(info));
    if (id) {
        createPools(*id);
    }
    return id;
}

std::optional<ComponentId> Components::find(std::string_view name) const noexcept {
    if (const ComponentInfo* info = _registry.findByName(name)) {
        return info->type.id;
    }
    return std::nullopt;
}

bool Components::add(Entity entity, ComponentId id, std::span<const std::byte> value) {
    storage::SparseSetStorage* pool = findPool(id);
    if (pool == nullptr || !isAlive(entity) || value.size() != pool->getInfo().type.size) {
        return false;
    }
    if (pool->contains(entity)) {
        pool->replaceCopy(entity, value, getChangeTick());
    } else {
        pool->emplaceCopy(entity, value, getChangeTick());
    }
    return true;
}

bool Components::addMove(Entity entity, ComponentId id, std::span<std::byte> value) {
    storage::SparseSetStorage* pool = findPool(id);
    if (pool == nullptr || !isAlive(entity) || value.size() != pool->getInfo().type.size) {
        return false;
    }
    if (pool->contains(entity)) {
        pool->replaceMove(entity, value, getChangeTick());
    } else {
        pool->emplaceMove(entity, value, getChangeTick());
    }
    return true;
}

bool Components::has(Entity entity, ComponentId id) const noexcept {
    const storage::SparseSetStorage* pool = findPool(id);
    return pool != nullptr && pool->contains(entity);
}

std::span<const std::byte> Components::get(Entity entity, ComponentId id) const noexcept {
    const storage::SparseSetStorage* pool = findPool(id);
    return pool != nullptr ? pool->get(entity) : std::span<const std::byte>{};
}

std::span<std::byte> Components::getMut(Entity entity, ComponentId id) noexcept {
    storage::SparseSetStorage* pool = findPool(id);
    if (pool == nullptr) {
        return {};
    }
    const std::optional<std::uint32_t> position = pool->find(entity);
    if (!position) {
        return {};
    }
    *std::next(pool->getChangedTicks().begin(), *position) = getChangeTick();
    return pool->at(*position);
}

bool Components::remove(Entity entity, ComponentId id) noexcept {
    storage::SparseSetStorage* pool = findPool(id);
    return pool != nullptr && pool->remove(entity);
}

std::optional<ComponentTicks> Components::getTicks(Entity entity, ComponentId id) const noexcept {
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

void Components::removeAll(Entity entity) noexcept {
    // Every pool is checked: an O(1) lookup each. A per-entity list of its pools may replace the scan later.
    for (storage::SparseSetStorage& pool : _pools) {
        pool.remove(entity);
    }
}

void Components::clearReferencesTo(Entity entity) noexcept {
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
                    *std::next(pool.getChangedTicks().begin(), static_cast<std::ptrdiff_t>(position)) = getChangeTick();
                }
            }
        }
    }
}

void Components::clampTicks(Tick now) noexcept {
    for (storage::SparseSetStorage& pool : _pools) {
        for (Tick& tick : pool.getAddedTicks()) {
            clampTick(tick, now);
        }
        for (Tick& tick : pool.getChangedTicks()) {
            clampTick(tick, now);
        }
    }
}

bool Components::isAlive(Entity entity) const noexcept { return _state->entities.isAlive(entity); }

Tick Components::getChangeTick() const noexcept { return _state->changeTick; }

Result<void> Components::checkNameFree(std::string_view name) const {
    return _state->checkNameFree(name, TypeKind::kComponent);
}

void Components::createPools(ComponentId id) {
    // A loop, not a single push: if creating a pool ever threw, the next registration catches up.
    while (_pools.size() <= id) {
        _pools.emplace_back(*_registry.findById(static_cast<ComponentId>(_pools.size())));
    }
}

storage::SparseSetStorage* Components::findPool(ComponentId id) noexcept {
    return id < _pools.size() ? &*std::next(_pools.begin(), id) : nullptr;
}

const storage::SparseSetStorage* Components::findPool(ComponentId id) const noexcept {
    return id < _pools.size() ? &*std::next(_pools.begin(), id) : nullptr;
}
}  // namespace rtype::ecs
