/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#include <cstddef>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

#include "rtype/ecs/storage/ViewAs.hpp"

namespace rtype::ecs {
template <Component T>
ComponentId World::component(ComponentOptions options) {
    const ComponentId id = _registry.registerComponent<T>(options);
    createPools(id);
    return id;
}

template <Component First, Component... Rest>
Entity World::spawn(First first, Rest... rest) {
    const Entity entity = spawn();
    try {
        add(entity, std::move(first));
        (add(entity, std::move(rest)), ...);
    } catch (...) {
        despawn(entity);  // destroys the components already added
        throw;
    }
    return entity;
}

template <Component T>
bool World::add(Entity entity, T value) {
    if (!isAlive(entity)) {
        return false;
    }
    const ComponentInfo* info = _registry.find<T>();
    storage::SparseSetStorage& pool = *findPool(info != nullptr ? info->id : component<T>());
    const std::span<std::byte> bytes = std::as_writable_bytes(std::span{&value, 1}).first(pool.getInfo().size);
    if (pool.contains(entity)) {
        pool.replaceMove(entity, bytes, _changeTick);
    } else {
        pool.emplaceMove(entity, bytes, _changeTick);
    }
    return true;  // `value`, now moved-from, is destroyed when this function returns
}

template <Component T>
bool World::has(Entity entity) const noexcept {
    const ComponentInfo* info = _registry.find<T>();
    return info != nullptr && has(entity, info->id);
}

template <Component T>
const T* World::get(Entity entity) const noexcept {
    static_assert(!std::is_empty_v<T>, "a tag has no value: test it with has<T>()");
    const ComponentInfo* info = _registry.find<T>();
    if (info == nullptr) {
        return nullptr;
    }
    const std::span<const std::byte> bytes = get(entity, info->id);
    return bytes.empty() ? nullptr : storage::viewAs<T>(bytes);
}

template <Component T>
bool World::remove(Entity entity) noexcept {
    const ComponentInfo* info = _registry.find<T>();
    return info != nullptr && remove(entity, info->id);
}

template <Component T>
std::optional<ComponentTicks> World::getTicks(Entity entity) const noexcept {
    const ComponentInfo* info = _registry.find<T>();
    return info != nullptr ? getTicks(entity, info->id) : std::nullopt;
}
}  // namespace rtype::ecs
