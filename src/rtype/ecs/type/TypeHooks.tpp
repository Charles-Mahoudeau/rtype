/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeHooks
*/

#include <memory>
#include <type_traits>
#include <utility>

namespace rtype::ecs::detail {
template <typename T>
constexpr TypeHooks makeHooks() noexcept {
    TypeHooks hooks{};
    if constexpr (std::is_default_constructible_v<T> && !std::is_trivially_default_constructible_v<T>) {
        // Value-initializes, which applies default member initializers.
        hooks.construct = [](void* destination) { std::construct_at(static_cast<T*>(destination)); };
    }
    if constexpr (!std::is_trivially_destructible_v<T>) {
        hooks.destruct = [](void* target) noexcept { std::destroy_at(static_cast<T*>(target)); };
    }
    if constexpr (!std::is_trivially_copyable_v<T>) {
        // The moved-from source stays valid and is destroyed separately.
        hooks.move = [](void* destination, void* source) noexcept {
            std::construct_at(static_cast<T*>(destination), std::move(*static_cast<T*>(source)));
        };
        if constexpr (std::is_copy_constructible_v<T>) {
            hooks.copy = [](void* destination, const void* source) {
                std::construct_at(static_cast<T*>(destination), *static_cast<const T*>(source));
            };
        }
    }
    return hooks;
}
}  // namespace rtype::ecs::detail
