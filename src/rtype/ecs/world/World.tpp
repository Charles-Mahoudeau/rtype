/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <type_traits>
#include <utility>

#include "rtype/ecs/exceptions/MissingResourceException.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"

namespace rtype::ecs {
template <Component T>
ComponentId World::component(ComponentOptions options) {
    const ComponentId id = _registry.registerType<T>(options);
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
    storage::SparseSetStorage& pool = *findPool(info != nullptr ? info->type.id : component<T>());
    const std::span<std::byte> bytes = std::as_writable_bytes(std::span{&value, 1}).first(pool.getInfo().type.size);
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
    return info != nullptr && has(entity, info->type.id);
}

template <Component T>
const T* World::get(Entity entity) const noexcept {
    static_assert(!std::is_empty_v<T>, "a tag has no value: test it with has<T>()");
    const ComponentInfo* info = _registry.find<T>();
    if (info == nullptr) {
        return nullptr;
    }
    const std::span<const std::byte> bytes = get(entity, info->type.id);
    return bytes.empty() ? nullptr : storage::viewAs<T>(bytes);
}

template <Component T>
Mut<T> World::getMut(Entity entity) noexcept {
    static_assert(!std::is_empty_v<T>, "a tag has no value: test it with has<T>()");
    const ComponentInfo* info = _registry.find<T>();
    if (info == nullptr) {
        return {};
    }
    storage::SparseSetStorage& pool = *findPool(info->type.id);
    const std::optional<std::uint32_t> position = pool.find(entity);
    if (!position) {
        return {};
    }
    return Mut<T>{storage::viewAs<T>(pool.at(*position)), &*std::next(pool.getChangedTicks().begin(), *position),
                  _changeTick};
}

template <Component T>
bool World::remove(Entity entity) noexcept {
    const ComponentInfo* info = _registry.find<T>();
    return info != nullptr && remove(entity, info->type.id);
}

template <Component T>
std::optional<ComponentTicks> World::getTicks(Entity entity) const noexcept {
    const ComponentInfo* info = _registry.find<T>();
    return info != nullptr ? getTicks(entity, info->type.id) : std::nullopt;
}

template <Resource T>
ResourceId World::resource(ResourceOptions options) {
    const ResourceId id = _resourceRegistry.registerType<T>(options);
    createResourceStorages(id);
    return id;
}

template <Resource T>
void World::insertResource(T value) {
    insertResourceWith(std::move(value), ResourceOptions{.nonSend = false});
}

template <Resource T>
void World::insertNonSendResource(T value) {
    insertResourceWith(std::move(value), ResourceOptions{.nonSend = true});
}

template <Resource T>
bool World::hasResource() const noexcept {
    const ResourceInfo* info = _resourceRegistry.find<T>();
    return info != nullptr && hasResource(info->type.id);
}

template <Resource T>
const T* World::getResource() const noexcept {
    static_assert(!std::is_empty_v<T>, "an empty resource has no value: test it with hasResource<T>()");
    const ResourceInfo* info = _resourceRegistry.find<T>();
    if (info == nullptr) {
        return nullptr;
    }
    const std::span<const std::byte> bytes = getResource(info->type.id);
    return bytes.empty() ? nullptr : storage::viewAs<T>(bytes);
}

template <Resource T>
Mut<T> World::getResourceMut() noexcept {
    static_assert(!std::is_empty_v<T>, "an empty resource has no value: test it with hasResource<T>()");
    const ResourceInfo* info = _resourceRegistry.find<T>();
    if (info == nullptr) {
        return {};
    }
    storage::ResourceStorage& resource = *findResourceStorage(info->type.id);
    if (!resource.hasValue()) {
        return {};
    }
    return Mut<T>{storage::viewAs<T>(resource.get()), &resource.getTicks().changed, _changeTick};
}

template <Resource T>
const T& World::requireResource() const {
    if (const T* value = getResource<T>()) {
        return *value;
    }
    throw exceptions::MissingResourceException{ResourceTraits<T>::kName};
}

template <Resource T>
Mut<T> World::requireResourceMut() {
    if (Mut<T> value = getResourceMut<T>()) {
        return value;
    }
    throw exceptions::MissingResourceException{ResourceTraits<T>::kName};
}

template <Resource T>
bool World::removeResource() noexcept {
    const ResourceInfo* info = _resourceRegistry.find<T>();
    return info != nullptr && removeResource(info->type.id);
}

template <Resource T>
std::optional<ComponentTicks> World::getResourceTicks() const noexcept {
    const ResourceInfo* info = _resourceRegistry.find<T>();
    return info != nullptr ? getResourceTicks(info->type.id) : std::nullopt;
}

template <Resource T>
void World::insertResourceWith(T value, ResourceOptions options) {
    const ResourceInfo* info = _resourceRegistry.find<T>();
    const ResourceId id = info != nullptr && info->nonSend == options.nonSend ? info->type.id : resource<T>(options);
    storage::ResourceStorage& resource = *findResourceStorage(id);
    resource.insertMove(std::as_writable_bytes(std::span{&value, 1}).first(resource.getInfo().type.size), _changeTick);
}
}  // namespace rtype::ecs
