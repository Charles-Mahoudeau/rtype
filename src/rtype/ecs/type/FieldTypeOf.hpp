/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldTypeOf
*/

#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <type_traits>

#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"

namespace rtype::ecs {
/// @brief Maps a C++ type to the FieldType it is reflected as, in `value`. Empty for unsupported types.
///
/// @details The ECS maps the scalar types itself. It is also the extension point for other types: specialize it,
/// at global scope or in `rtype::ecs`, to reflect a math type of your own. The type must have the size and
/// alignment of the FieldType (see getFieldLayout()).
/// @code
/// template <>
/// struct rtype::ecs::FieldTypeOf<glm::vec2>
///     : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kVec2> {};
/// @endcode
template <typename T>
struct FieldTypeOf {};

template <>
struct FieldTypeOf<bool> : std::integral_constant<FieldType, FieldType::kBool> {};
template <>
struct FieldTypeOf<std::int8_t> : std::integral_constant<FieldType, FieldType::kI8> {};
template <>
struct FieldTypeOf<std::int16_t> : std::integral_constant<FieldType, FieldType::kI16> {};
template <>
struct FieldTypeOf<std::int32_t> : std::integral_constant<FieldType, FieldType::kI32> {};
template <>
struct FieldTypeOf<std::int64_t> : std::integral_constant<FieldType, FieldType::kI64> {};
template <>
struct FieldTypeOf<std::uint8_t> : std::integral_constant<FieldType, FieldType::kU8> {};
template <>
struct FieldTypeOf<std::uint16_t> : std::integral_constant<FieldType, FieldType::kU16> {};
template <>
struct FieldTypeOf<std::uint32_t> : std::integral_constant<FieldType, FieldType::kU32> {};
template <>
struct FieldTypeOf<std::uint64_t> : std::integral_constant<FieldType, FieldType::kU64> {};
template <>
struct FieldTypeOf<float> : std::integral_constant<FieldType, FieldType::kF32> {};
template <>
struct FieldTypeOf<double> : std::integral_constant<FieldType, FieldType::kF64> {};

/// @brief A reference to another entity: cleared when that entity is despawned, remapped by the network.
template <>
struct FieldTypeOf<Entity> : std::integral_constant<FieldType, FieldType::kEntity> {};

/// @brief True when a FieldTypeOf specialization reflects `T`.
template <typename T>
concept HasFieldType = requires {
    { FieldTypeOf<T>::value } -> std::convertible_to<FieldType>;
};

namespace detail {
/// @brief Splits a field's declared type into the type of one value and the number of values.
template <typename T>
struct FieldElement {
    using Type = T;                             ///< Type of one value.
    static constexpr std::uint32_t kCount = 1;  ///< Number of values.
};

/// @brief A `std::array` field: `std::array<float, 4>` is 4 values of `float`. C arrays are not supported.
template <typename T, std::size_t N>
struct FieldElement<std::array<T, N>> {
    using Type = T;
    static constexpr auto kCount = static_cast<std::uint32_t>(N);
};
}  // namespace detail
}  // namespace rtype::ecs
