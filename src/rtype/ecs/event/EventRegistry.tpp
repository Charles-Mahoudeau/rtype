/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventRegistry
*/

namespace rtype::ecs {
template <Event T>
EventInfo KindTraits<EventInfo>::make(const EventOptions& options) {
    EventInfo info = EventTraits<T>::makeInfo();
    info.type.presence = options.presence;
    return info;
}
}  // namespace rtype::ecs
