/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputKind
*/

#pragma once

#include <cstdint>

namespace rtype::replication::input {
/// @brief What an input action carries. Wire values: 0 is never valid.
enum class InputKind : std::uint8_t {
    kButton = 1,   ///< Held or not: one bit.
    kAxis = 2,     ///< One value in [-1, 1].
    kVector2 = 3,  ///< Two values in [-1, 1].
};

/// @brief Tells whether `kind` is a value of the wire (1 to 3, never 0); found by argument-dependent lookup by the
/// streams.
[[nodiscard]] constexpr bool isValidWireValue(const InputKind kind) noexcept {
    return kind >= InputKind::kButton && kind <= InputKind::kVector2;
}
}  // namespace rtype::replication::input
