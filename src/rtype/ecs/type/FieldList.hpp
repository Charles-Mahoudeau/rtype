/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldList
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
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

/// @brief The ListAccess of a field of type `List`: a `std::vector` or a `std::string`.
template <typename List>
[[nodiscard]] consteval ListAccess makeListAccess() noexcept;

/// @brief The one ListAccess of the fields of type `List`, so that a field can point to it.
template <typename List>
inline constexpr ListAccess kListAccess = makeListAccess<List>();

/// @brief One byte of a `std::string` field, reflected as FieldType::kText. A type of its own, so that a `char` field
/// stays refused: kText only exists inside a text.
struct TextByte {};

/// @brief A `std::vector` field: a list of values of `T`, which must be plain data.
template <typename T>
struct FieldElement<std::vector<T>> {
    using Type = T;
    static constexpr std::uint32_t kCount = 1;
    static constexpr const ListAccess* kList = &kListAccess<std::vector<T>>;
};

/// @brief A `std::string` field: a text of any length, which is a list of bytes with its own FieldType.
template <typename Traits, typename Allocator>
struct FieldElement<std::basic_string<char, Traits, Allocator>> {
    using Type = TextByte;
    static constexpr std::uint32_t kCount = 1;
    static constexpr const ListAccess* kList = &kListAccess<std::basic_string<char, Traits, Allocator>>;
};
}  // namespace rtype::ecs::detail

#define RTYPE_FIELD_LIST_INCLUDING_TPP
#include "FieldList.tpp"  // IWYU pragma: export
#undef RTYPE_FIELD_LIST_INCLUDING_TPP
