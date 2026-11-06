/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallTraits
*/

#pragma once

#ifndef RTYPE_ECS_CALL_TRAITS_INCLUDING_TPP
#error "CallTraits.tpp is an implementation detail of CallTraits.hpp: include CallTraits.hpp instead"
#endif

#include <string_view>
#include <type_traits>

#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::ecs::detail {
template <typename T, typename... Fields>
constexpr TypeDescription<sizeof...(Fields)> describeCallData(std::string_view name, Fields... fields) {
    static_assert(std::is_object_v<T> && !std::is_array_v<T> && !std::is_const_v<T> && !std::is_volatile_v<T>,
                  "RTYPE_ECS_CALL_DATA: call data must be a plain, non-const object type.");
    static_assert(std::is_standard_layout_v<T>,
                  "RTYPE_ECS_CALL_DATA: call data must be standard-layout (all members public, no reference members, "
                  "no virtual base).");
    static_assert(std::is_trivially_copyable_v<T> && std::is_trivially_destructible_v<T>,
                  "RTYPE_ECS_CALL_DATA: call data must be plain data (trivially copyable and destructible): it is "
                  "copied as bytes.");
    static_assert(std::is_default_constructible_v<T>,
                  "RTYPE_ECS_CALL_DATA: call data must be default-constructible: a script leaves out the fields that "
                  "keep their default.");
    return TypeDescription<sizeof...(Fields)>{.name = name, .kind = TypeKind::kCallData, .fields = {fields...}};
}
}  // namespace rtype::ecs::detail
