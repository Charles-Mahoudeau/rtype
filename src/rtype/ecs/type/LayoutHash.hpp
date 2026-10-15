/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LayoutHash
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "rtype/ecs/type/TypeHooks.hpp"

namespace rtype::ecs {
/// @brief Fingerprint of a type's layout: its name, size, alignment, which lifecycle hooks it has, whether it is
/// copyable, and every field (name, type, offset, count).
///
/// @details The one implementation behind computeLayoutHash() (at run time, from a ComponentInfo) and
/// ComponentTraits::kLayoutHash (at compile time, from what RTYPE_ECS_COMPONENT recorded), so both always agree.
/// Only whether each hook exists is hashed, not its address: a component that needs constructing, copying or
/// destroying is not byte-compatible with one that does not, while addresses differ between libraries.
/// @param fields A range of FieldInfo or FieldDescription: anything whose elements have a name, a type, an offset
/// and a count.
template <typename Fields>
[[nodiscard]] constexpr std::uint64_t hashLayout(std::string_view name, std::size_t size, std::size_t alignment,
                                                 const TypeHooks& hooks, bool copyable, const Fields& fields) noexcept;
}  // namespace rtype::ecs

#include "LayoutHash.tpp"
