/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentRegistry
*/

#include <string>
#include <utility>

#include "rtype/ecs/exceptions/ComponentExceptions.hpp"

namespace rtype::ecs {
template <Component T>
ComponentId ComponentRegistry::registerComponent(ComponentOptions options) {
    ComponentInfo info = ComponentTraits<T>::makeInfo();
    info.storage = options.storage;
    info.type.presence = options.presence;
    info.type.replication = options.replication;
    Result<ComponentId> id = registerComponent(std::move(info));
    if (!id) {
        throw exceptions::ComponentRegistrationException{std::string{id.error().getMessage()}};
    }
    return *id;
}

template <Component T>
const ComponentInfo* ComponentRegistry::find() const noexcept {
    const ComponentInfo* info = findByHash(ComponentTraits<T>::kHash);
    if (info == nullptr || info->type.name != ComponentTraits<T>::kName ||
        info->type.layoutHash != ComponentTraits<T>::kLayoutHash) {
        return nullptr;
    }
    return info;
}
}  // namespace rtype::ecs
