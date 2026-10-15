/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeHooks
*/

#pragma once

namespace rtype::ecs {
/// @brief Type-erased lifecycle of the bytes of a component or a resource. A null hook means trivial: zero-fill, memcpy
/// or nothing.
///
/// @details Hooks only manage the value's own memory (a C++ type holding a `std::vector`); gameplay reactions belong
/// in systems. They are null for plain data and always null for Luau types. Moving and
/// destroying cannot throw: their pointer types are `noexcept`.
struct TypeHooks {
    void (*construct)(void* destination) = nullptr;                    ///< Default-constructs in raw storage.
    void (*destruct)(void* target) noexcept = nullptr;                 ///< Destroys a value.
    void (*move)(void* destination, void* source) noexcept = nullptr;  ///< Move-constructs into raw storage.
    void (*copy)(void* destination, const void* source) = nullptr;     ///< Copy-constructs into raw storage.
};

namespace detail {
/// @brief Builds the lifecycle hooks of `T`: `nullptr` for every operation that is trivial, or that `T` does not
/// support (no default constructor, not copyable), so the same builder serves components and resources.
template <typename T>
[[nodiscard]] constexpr TypeHooks makeHooks() noexcept;
}  // namespace detail
}  // namespace rtype::ecs

#include "TypeHooks.tpp"
