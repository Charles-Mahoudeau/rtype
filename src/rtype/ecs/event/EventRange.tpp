/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventRange
*/

#include <cstddef>
#include <type_traits>

#include "rtype/ecs/storage/ViewAs.hpp"

namespace rtype::ecs {
template <Event T>
EventRange<T>::Iterator::Iterator(RawEventRange::Iterator raw) noexcept : _raw{raw} {}

template <Event T>
const T& EventRange<T>::Iterator::operator*() const noexcept {
    static_assert(!std::is_empty_v<T>, "an event type without data has no value: count its events with size()");
    return *storage::viewAs<T>(*_raw);
}

template <Event T>
const T* EventRange<T>::Iterator::operator->() const noexcept {
    return &**this;
}

template <Event T>
auto EventRange<T>::Iterator::operator++() noexcept -> Iterator& {
    ++_raw;
    return *this;
}

template <Event T>
bool EventRange<T>::Iterator::operator==(const Iterator& other) const noexcept {
    return _raw == other._raw;
}

template <Event T>
EventRange<T>::EventRange(RawEventRange raw) noexcept : _raw{raw} {}

template <Event T>
std::size_t EventRange<T>::size() const noexcept {
    return _raw.size();
}

template <Event T>
bool EventRange<T>::empty() const noexcept {
    return _raw.empty();
}

template <Event T>
auto EventRange<T>::begin() const noexcept -> Iterator {
    return Iterator{_raw.begin()};
}

template <Event T>
auto EventRange<T>::end() const noexcept -> Iterator {
    return Iterator{_raw.end()};
}
}  // namespace rtype::ecs
