/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Mut
*/

#include <cassert>

namespace rtype::ecs {
template <typename T>
Mut<T>::Mut(T* value, Tick* changed, Tick tick) noexcept : _value{value}, _changed{changed}, _tick{tick} {}

template <typename T>
const T& Mut<T>::get() const noexcept {
    assert(_value != nullptr);
    return *_value;
}

template <typename T>
T* Mut<T>::operator->() noexcept {
    markChanged();
    return _value;
}

template <typename T>
T& Mut<T>::operator*() noexcept {
    markChanged();
    return *_value;
}

template <typename T>
Mut<T>::operator bool() const noexcept {
    return _value != nullptr;
}

template <typename T>
void Mut<T>::markChanged() noexcept {
    assert(_value != nullptr && _changed != nullptr);
    *_changed = _tick;
}
}  // namespace rtype::ecs
