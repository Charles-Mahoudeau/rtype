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
    info.presence = options.presence;
    info.replication = options.replication;
    Result<ComponentId> id = registerComponent(std::move(info));
    if (!id) {
        throw exceptions::ComponentRegistrationException{std::string{id.error().getMessage()}};
    }
    return *id;
}

template <Component T>
const ComponentInfo* ComponentRegistry::find() const noexcept {
    const ComponentInfo* info = findByHash(ComponentTraits<T>::kHash);
    return info != nullptr && info->name == ComponentTraits<T>::kName ? info : nullptr;
}
}  // namespace rtype::ecs
