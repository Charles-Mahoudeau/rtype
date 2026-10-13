/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ViewAs
*/

#pragma once

#include <cstddef>
#include <span>

namespace rtype::ecs::storage {
/// @brief Views the bytes of a value that already lives in storage as that value.
///
/// @details The bytes must hold a live `T`: one created by a construct, copy or move hook, or written with
/// `memset` / `memcpy` into allocated memory, which is how a ByteColumn creates its values. It does not create an
/// object from arbitrary bytes, and does not check that `T` is the stored type: only its size and alignment.
/// @code
/// Health* health = rtype::ecs::storage::viewAs<Health>(column.at(0));
/// @endcode
/// @param bytes Exactly `sizeof(T)` bytes, aligned for `T` (checked by `assert`). Not usable with tags, whose
/// values have no bytes.
/// @return The value held by `bytes`.
template <typename T>
[[nodiscard]] T* viewAs(std::span<std::byte> bytes) noexcept;

/// @brief Views the bytes of a value that already lives in storage as that value, read-only.
/// @see viewAs(std::span<std::byte>)
template <typename T>
[[nodiscard]] const T* viewAs(std::span<const std::byte> bytes) noexcept;
}  // namespace rtype::ecs::storage

#include "ViewAs.tpp"
