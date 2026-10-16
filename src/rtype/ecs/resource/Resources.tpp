/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Resources
*/

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <type_traits>
#include <utility>

#include "rtype/ecs/exceptions/MissingResourceException.hpp"
#include "rtype/ecs/exceptions/TypeExceptions.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"

namespace rtype::ecs {
template <Resource T>
ResourceId Resources::registerType(ResourceOptions options) {
    if (Result<void> free = checkNameFree(ResourceTraits<T>::kName); !free) {
        throw exceptions::TypeRegistrationException{std::string{free.error().getMessage()}};
    }
    const ResourceId id = _registry.registerType<T>(options);
    createStorages(id);
    return id;
}

template <Resource T>
void Resources::insert(T value) {
    insertWith(std::move(value), ResourceOptions{.nonSend = false});
}

template <Resource T>
void Resources::insertNonSend(T value) {
    insertWith(std::move(value), ResourceOptions{.nonSend = true});
}

template <Resource T>
bool Resources::has() const noexcept {
    const ResourceInfo* info = _registry.find<T>();
    return info != nullptr && has(info->type.id);
}

template <Resource T>
const T* Resources::get() const noexcept {
    static_assert(!std::is_empty_v<T>, "an empty resource has no value: test it with has<T>()");
    const ResourceInfo* info = _registry.find<T>();
    if (info == nullptr) {
        return nullptr;
    }
    const std::span<const std::byte> bytes = get(info->type.id);
    return bytes.empty() ? nullptr : storage::viewAs<T>(bytes);
}

template <Resource T>
Mut<T> Resources::getMut() noexcept {
    static_assert(!std::is_empty_v<T>, "an empty resource has no value: test it with has<T>()");
    const ResourceInfo* info = _registry.find<T>();
    if (info == nullptr) {
        return {};
    }
    storage::ResourceStorage& resource = *findStorage(info->type.id);
    if (!resource.hasValue()) {
        return {};
    }
    return Mut<T>{storage::viewAs<T>(resource.get()), &resource.getTicks().changed, getChangeTick()};
}

template <Resource T>
const T& Resources::require() const {
    if (const T* value = get<T>()) {
        return *value;
    }
    throw exceptions::MissingResourceException{ResourceTraits<T>::kName};
}

template <Resource T>
Mut<T> Resources::requireMut() {
    if (Mut<T> value = getMut<T>()) {
        return value;
    }
    throw exceptions::MissingResourceException{ResourceTraits<T>::kName};
}

template <Resource T>
bool Resources::remove() noexcept {
    const ResourceInfo* info = _registry.find<T>();
    return info != nullptr && remove(info->type.id);
}

template <Resource T>
std::optional<ComponentTicks> Resources::getTicks() const noexcept {
    const ResourceInfo* info = _registry.find<T>();
    return info != nullptr ? getTicks(info->type.id) : std::nullopt;
}

template <Resource T>
void Resources::insertWith(T value, ResourceOptions options) {
    const ResourceInfo* info = _registry.find<T>();
    const ResourceId id =
        info != nullptr && info->nonSend == options.nonSend ? info->type.id : registerType<T>(options);
    storage::ResourceStorage& resource = *findStorage(id);
    resource.insertMove(std::as_writable_bytes(std::span{&value, 1}).first(resource.getInfo().type.size),
                        getChangeTick());
}
}  // namespace rtype::ecs
