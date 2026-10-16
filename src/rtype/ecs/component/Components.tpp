/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Components
*/

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <span>
#include <string>
#include <type_traits>

#include "rtype/ecs/exceptions/TypeExceptions.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"

namespace rtype::ecs {
template <Component T>
ComponentId Components::registerType(ComponentOptions options) {
    if (Result<void> free = checkNameFree(ComponentTraits<T>::kName); !free) {
        throw exceptions::TypeRegistrationException{std::string{free.error().getMessage()}};
    }
    const ComponentId id = _registry.registerType<T>(options);
    createPools(id);
    return id;
}

template <Component T>
bool Components::add(Entity entity, T value) {
    if (!isAlive(entity)) {
        return false;
    }
    const ComponentInfo* info = _registry.find<T>();
    storage::SparseSetStorage& pool = *findPool(info != nullptr ? info->type.id : registerType<T>());
    const std::span<std::byte> bytes = std::as_writable_bytes(std::span{&value, 1}).first(pool.getInfo().type.size);
    if (pool.contains(entity)) {
        pool.replaceMove(entity, bytes, getChangeTick());
    } else {
        pool.emplaceMove(entity, bytes, getChangeTick());
    }
    return true;  // `value`, now moved-from, is destroyed when this function returns
}

template <Component T>
bool Components::has(Entity entity) const noexcept {
    const ComponentInfo* info = _registry.find<T>();
    return info != nullptr && has(entity, info->type.id);
}

template <Component T>
const T* Components::get(Entity entity) const noexcept {
    static_assert(!std::is_empty_v<T>, "a tag has no value: test it with has<T>()");
    const ComponentInfo* info = _registry.find<T>();
    if (info == nullptr) {
        return nullptr;
    }
    const std::span<const std::byte> bytes = get(entity, info->type.id);
    return bytes.empty() ? nullptr : storage::viewAs<T>(bytes);
}

template <Component T>
Mut<T> Components::getMut(Entity entity) noexcept {
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
                  getChangeTick()};
}

template <Component T>
bool Components::remove(Entity entity) noexcept {
    const ComponentInfo* info = _registry.find<T>();
    return info != nullptr && remove(entity, info->type.id);
}

template <Component T>
std::optional<ComponentTicks> Components::getTicks(Entity entity) const noexcept {
    const ComponentInfo* info = _registry.find<T>();
    return info != nullptr ? getTicks(entity, info->type.id) : std::nullopt;
}
}  // namespace rtype::ecs
