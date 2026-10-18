/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventWriter
*/

#include <utility>

namespace rtype::ecs {
template <Event T>
EventWriter<T>::EventWriter(Events& events) noexcept : _events{&events} {}

template <Event T>
void EventWriter<T>::send(T event) {
    _events->send(std::move(event));
}
}  // namespace rtype::ecs
