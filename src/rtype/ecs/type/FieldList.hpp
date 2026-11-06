/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldList
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "rtype/ecs/type/Field.hpp"

namespace rtype::ecs::detail {
/// @brief Splits a field's declared type into the type of one value and the number of values.
template <typename T>
struct FieldElement {
    using Type = T;                                      ///< Type of one value.
    static constexpr std::uint32_t kCount = 1;           ///< Number of values.
    static constexpr const ListAccess* kList = nullptr;  ///< How to reach a list's values; null for other fields.
};

/// @brief The ListAccess of a `std::vector<T>` field.
template <typename T>
[[nodiscard]] consteval ListAccess makeListAccess() noexcept;

/// @brief The one ListAccess of `std::vector<T>` fields, so that a field can point to it.
template <typename T>
inline constexpr ListAccess kListAccess = makeListAccess<T>();

/// @brief A `std::vector` field: a list of values of `T`, which must be plain data.
template <typename T>
struct FieldElement<std::vector<T>> {
    using Type = T;
    static constexpr std::uint32_t kCount = 1;
    static constexpr const ListAccess* kList = &kListAccess<T>;
};
}  // namespace rtype::ecs::detail

#define RTYPE_FIELD_LIST_INCLUDING_TPP
#include "FieldList.tpp"  // IWYU pragma: export
#undef RTYPE_FIELD_LIST_INCLUDING_TPP
