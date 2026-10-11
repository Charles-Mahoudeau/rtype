/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NameHash
*/

#pragma once

#include <cstdint>
#include <string_view>

namespace rtype::ecs {
/// @brief Starting value of the FNV-1a 64-bit hash.
inline constexpr std::uint64_t kNameHashOffsetBasis = 14695981039346656037ULL;

/// @brief Multiplier of the FNV-1a 64-bit hash.
inline constexpr std::uint64_t kNameHashPrime = 1099511628211ULL;

/// @brief Hashes a component name (FNV-1a, 64 bits), at compile time or at run time.
///
/// @details The hash only depends on the characters, so it is the same in every compiler, library and process: it
/// identifies a component across shared libraries, where a per-type counter would not.
[[nodiscard]] constexpr std::uint64_t hashName(std::string_view name) noexcept {
    std::uint64_t hash = kNameHashOffsetBasis;
    for (const char character : name) {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= kNameHashPrime;
    }
    return hash;
}
}  // namespace rtype::ecs
