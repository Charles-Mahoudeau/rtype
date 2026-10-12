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

/// @brief Continues an FNV-1a 64-bit hash with the characters of a string.
/// @param hash The hash so far: kNameHashOffsetBasis to start a new one.
[[nodiscard]] constexpr std::uint64_t hashString(std::uint64_t hash, std::string_view text) noexcept {
    for (const char character : text) {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= kNameHashPrime;
    }
    return hash;
}

/// @brief Continues an FNV-1a 64-bit hash with the 8 bytes of an integer, lowest byte first.
///
/// @details The byte order is fixed rather than taken from memory, so the result is the same on every platform.
/// @param hash The hash so far: kNameHashOffsetBasis to start a new one.
[[nodiscard]] constexpr std::uint64_t hashInteger(std::uint64_t hash, std::uint64_t value) noexcept {
    for (std::uint32_t byte = 0; byte < sizeof(value); ++byte) {
        hash ^= (value >> (byte * 8U)) & 0xFFU;
        hash *= kNameHashPrime;
    }
    return hash;
}

/// @brief Hashes a component name (FNV-1a, 64 bits), at compile time or at run time.
///
/// @details The hash only depends on the characters, so it is the same in every compiler, library and process: it
/// identifies a component across shared libraries, where a per-type counter would not.
[[nodiscard]] constexpr std::uint64_t hashName(std::string_view name) noexcept {
    return hashString(kNameHashOffsetBasis, name);
}
}  // namespace rtype::ecs
