/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** World
*/

#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

#include "rtype/ecs/exceptions/MissingResourceException.hpp"
#include "rtype/ecs/exceptions/TypeExceptions.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"

namespace rtype::ecs {
template <Component First, Component... Rest>
Entity World::spawn(First first, Rest... rest) {
    const Entity entity = spawn();
    try {
        _state->components.add(entity, std::move(first));
        (_state->components.add(entity, std::move(rest)), ...);
    } catch (...) {
        despawn(entity);  // destroys the components already added
        throw;
    }
    return entity;
}

template <Resource T>
ResourceId World::resource(ResourceOptions options) {
    if (Result<void> free = checkNameFree(ResourceTraits<T>::kName); !free) {
        throw exceptions::TypeRegistrationException{std::string{free.error().getMessage()}};
    }
    const ResourceId id = _state->resourceRegistry.registerType<T>(options);
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
    const ResourceInfo* info = _state->resourceRegistry.find<T>();
    return info != nullptr && hasResource(info->type.id);
}

template <Resource T>
const T* World::getResource() const noexcept {
    static_assert(!std::is_empty_v<T>, "an empty resource has no value: test it with hasResource<T>()");
    const ResourceInfo* info = _state->resourceRegistry.find<T>();
    if (info == nullptr) {
        return nullptr;
    }
    const std::span<const std::byte> bytes = getResource(info->type.id);
    return bytes.empty() ? nullptr : storage::viewAs<T>(bytes);
}

template <Resource T>
Mut<T> World::getResourceMut() noexcept {
    static_assert(!std::is_empty_v<T>, "an empty resource has no value: test it with hasResource<T>()");
    const ResourceInfo* info = _state->resourceRegistry.find<T>();
    if (info == nullptr) {
        return {};
    }
    storage::ResourceStorage& resource = *findResourceStorage(info->type.id);
    if (!resource.hasValue()) {
        return {};
    }
    return Mut<T>{storage::viewAs<T>(resource.get()), &resource.getTicks().changed, _state->changeTick};
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
    const ResourceInfo* info = _state->resourceRegistry.find<T>();
    return info != nullptr && removeResource(info->type.id);
}

template <Resource T>
std::optional<ComponentTicks> World::getResourceTicks() const noexcept {
    const ResourceInfo* info = _state->resourceRegistry.find<T>();
    return info != nullptr ? getResourceTicks(info->type.id) : std::nullopt;
}

template <Resource T>
void World::insertResourceWith(T value, ResourceOptions options) {
    const ResourceInfo* info = _state->resourceRegistry.find<T>();
    const ResourceId id = info != nullptr && info->nonSend == options.nonSend ? info->type.id : resource<T>(options);
    storage::ResourceStorage& resource = *findResourceStorage(id);
    resource.insertMove(std::as_writable_bytes(std::span{&value, 1}).first(resource.getInfo().type.size),
                        _state->changeTick);
}
}  // namespace rtype::ecs
