/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FieldList
*/

#pragma once

#ifndef RTYPE_FIELD_LIST_INCLUDING_TPP
#error "FieldList.tpp is an implementation detail of FieldList.hpp: include FieldList.hpp instead"
#endif

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <type_traits>
#include <vector>

namespace rtype::ecs::detail {
template <typename List>
consteval ListAccess makeListAccess() noexcept {
    using T = List::value_type;
    static_assert(!std::same_as<T, bool>, "RTYPE_ECS: std::vector<bool> holds no bools: use std::vector<std::uint8_t>");
    static_assert(std::is_trivially_copyable_v<T>, "RTYPE_ECS: the values of a list field must be plain data");
    return ListAccess{
        .size = static_cast<std::uint32_t>(sizeof(List)),
        .alignment = static_cast<std::uint32_t>(alignof(List)),
        .elements = [](const void* list) noexcept { return std::as_bytes(std::span{*static_cast<const List*>(list)}); },
        .assign =
            [](void* list, std::span<const std::byte> elements) {
                List& values = *static_cast<List*>(list);
                values.resize(elements.size() / sizeof(T));
                if (!elements.empty()) {
                    std::memcpy(values.data(), elements.data(), values.size() * sizeof(T));
                }
            },
        .erase =
            [](void* list, std::span<const std::byte> element) noexcept {
                if constexpr (std::equality_comparable<T> && !std::same_as<T, char>) {
                    T value{};
                    std::memcpy(&value, element.data(), sizeof(T));
                    std::erase(*static_cast<List*>(list), value);
                }
            },
    };
}
}  // namespace rtype::ecs::detail
