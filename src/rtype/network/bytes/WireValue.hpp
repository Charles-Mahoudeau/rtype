/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WireValue
*/

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace rtype::network::bytes {
namespace detail {
/// @brief The unsigned integer type of `N` bytes; a size with no such type is a compile error.
template <std::size_t N>
struct UnsignedOf;

template <>
struct UnsignedOf<1> {
    using Type = std::uint8_t;
};

template <>
struct UnsignedOf<2> {
    using Type = std::uint16_t;
};

template <>
struct UnsignedOf<4> {
    using Type = std::uint32_t;
};

template <>
struct UnsignedOf<8> {
    using Type = std::uint64_t;
};

/// @brief The unsigned integer type with the size of `T`: the type that carries the bits of a `T` on the wire.
template <typename T>
using UnsignedOfSize = UnsignedOf<sizeof(T)>::Type;
}  // namespace detail

/// @brief A wire enum: it declares `isValidWireValue(e)` next to itself (found by argument-dependent lookup), so
/// that no enum can be written or read without a range check.
template <typename T>
concept WireEnum = std::is_enum_v<T> && requires(T value) {
    { isValidWireValue(value) } -> std::convertible_to<bool>;
};

/// @brief A type the streams' `value()` handles: integers, `bool`, `float`, `double` and wire enums.
template <typename T>
concept WireValue = std::integral<T> || std::same_as<T, float> || std::same_as<T, double> || WireEnum<T>;

/// @brief The type of a list's element count on the wire.
template <typename T>
concept WireCount = std::same_as<T, std::uint8_t> || std::same_as<T, std::uint16_t>;
}  // namespace rtype::network::bytes
