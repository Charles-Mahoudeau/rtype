/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Events
*/

#include <cstddef>
#include <span>
#include <string>
#include <utility>

#include "rtype/ecs/exceptions/TypeExceptions.hpp"

namespace rtype::ecs {
template <Event T>
EventId Events::registerType(EventOptions options) {
    if (Result<void> free = checkNameFree(EventTraits<T>::kName); !free) {
        throw exceptions::TypeRegistrationException{std::string{free.error().getMessage()}};
    }
    const EventId id = _registry.registerType<T>(options);
    createStorages(id);
    return id;
}

template <Event T>
void Events::send(T value) {
    const EventInfo* info = _registry.find<T>();
    storage::EventStorage& queue = *findStorage(info != nullptr ? info->type.id : registerType<T>());
    queue.sendMove(std::as_writable_bytes(std::span{&value, 1}).first(queue.getInfo().type.size));
}

template <Event T>
EventRange<T> Events::read(EventCursor& cursor) const noexcept {
    const EventInfo* info = _registry.find<T>();
    return info != nullptr ? EventRange<T>{read(info->type.id, cursor)} : EventRange<T>{};
}

template <Event T>
std::size_t Events::getCount() const noexcept {
    const EventInfo* info = _registry.find<T>();
    return info != nullptr ? getCount(info->type.id) : 0;
}

template <Event T>
bool Events::update() noexcept {
    const EventInfo* info = _registry.find<T>();
    return info != nullptr && update(info->type.id);
}

template <Event T>
bool Events::clear() noexcept {
    const EventInfo* info = _registry.find<T>();
    return info != nullptr && clear(info->type.id);
}
}  // namespace rtype::ecs
