/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Value
*/

#pragma once

namespace rtype::luau {
template <typename T>
bool Value::is() const noexcept {
    return std::holds_alternative<T>(_variant);
}

template <typename T>
const T& Value::as() const {
    return std::get<T>(_variant);
}

template <typename T>
T& Value::as() {
    return std::get<T>(_variant);
}

template <typename T>
const T* Value::tryAs() const noexcept {
    return std::get_if<T>(&_variant);
}

template <typename T>
T* Value::tryAs() noexcept {
    return std::get_if<T>(&_variant);
}
}  // namespace rtype::luau
