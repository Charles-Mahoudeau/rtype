/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentRegistry
*/

namespace rtype::ecs {
template <Component T>
ComponentInfo KindTraits<ComponentInfo>::make(const ComponentOptions& options) {
    ComponentInfo info = ComponentTraits<T>::makeInfo();
    info.storage = options.storage;
    info.type.presence = options.presence;
    info.type.replication = options.replication;
    return info;
}
}  // namespace rtype::ecs
