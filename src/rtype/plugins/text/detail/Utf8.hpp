/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Utf8
*/

#pragma once

#include <string>
#include <string_view>

namespace rtype::plugins::text::detail {

/// @brief The character put in place of a byte sequence that is not UTF-8: U+FFFD.
inline constexpr char32_t kReplacementCharacter = 0xFFFD;

/// @return The characters of the UTF-8 @p text. Each byte that does not start a valid sequence (a stray continuation
/// byte, a truncated or overlong sequence, a surrogate, a value past U+10FFFF) becomes one kReplacementCharacter.
[[nodiscard]] std::u32string decodeUtf8(std::string_view text);

}  // namespace rtype::plugins::text::detail
