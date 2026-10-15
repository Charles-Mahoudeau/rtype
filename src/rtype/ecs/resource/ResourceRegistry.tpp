/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceRegistry
*/

namespace rtype::ecs {
template <Resource T>
ResourceInfo KindTraits<ResourceInfo>::make(const ResourceOptions& options) {
    ResourceInfo info = ResourceTraits<T>::makeInfo();
    info.type.presence = options.presence;
    info.type.replication = options.replication;
    info.nonSend = options.nonSend;
    return info;
}
}  // namespace rtype::ecs
