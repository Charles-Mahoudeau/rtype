/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** String
*/

#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

namespace rtype::ecs {
/// @brief A short UTF-8 text stored inline in a component: a length and a fixed buffer, so that it is plain data.
///
/// @details It has the same size (256 bytes) and alignment on every compiler, which `std::string` does not, and no
/// hook: it is copied as bytes, hashed in a layout and replicated like a number. The price is a capacity of
/// kCapacity bytes: a longer text is cut, never at the middle of a UTF-8 character.
class String {
  public:
    static constexpr std::size_t kCapacity = 255;  ///< The most bytes a String holds: its length is a `u8`.

    /// @brief Builds an empty string.
    constexpr String() noexcept = default;

    /// @brief Builds a string from a text, cut to kCapacity bytes.
    /// @param text The text.
    constexpr String(std::string_view text) noexcept {
        assign(text);
    }  // NOLINT(misc-explicit-constructor, cppcoreguidelines-explicit-constructor)

    /// @brief Builds a string from a C string, cut to kCapacity bytes.
    /// @param text A null-terminated text.
    constexpr String(
        const char* text) noexcept  // NOLINT(misc-explicit-constructor, cppcoreguidelines-explicit-constructor)
        : String{std::string_view{text}} {}

    /// @brief Builds a string from a `std::string`, cut to kCapacity bytes.
    /// @param text The text.
    String(const std::string& text) noexcept
        : String{std::string_view{text}} {
    }  // NOLINT(misc-explicit-constructor, cppcoreguidelines-explicit-constructor)

    constexpr ~String() noexcept = default;
    constexpr String(const String& other) noexcept = default;
    constexpr String& operator=(const String& other) noexcept = default;
    constexpr String(String&& other) noexcept = default;
    constexpr String& operator=(String&& other) noexcept = default;

    /// @return The number of bytes of the text.
    [[nodiscard]] constexpr std::size_t size() const noexcept { return _length; }

    /// @return True when the text has no byte.
    [[nodiscard]] constexpr bool empty() const noexcept { return _length == 0; }

    /// @return The text, valid as long as this string is unchanged.
    [[nodiscard]] constexpr std::string_view view() const noexcept { return {_bytes.data(), _length}; }

    /// @return The text, as a string view.
    [[nodiscard]] constexpr operator std::string_view() const noexcept { return view(); }  // NOLINT

    /// @brief Replaces the text, cut to kCapacity bytes, at a character boundary.
    /// @param text The new text.
    constexpr void assign(std::string_view text) noexcept {
        std::size_t length = std::min(text.size(), kCapacity);
        // A UTF-8 continuation byte is 0b10xxxxxx: step back to the start of the character that does not fit.
        while (length < text.size() && length > 0 && (static_cast<std::uint8_t>(text.at(length)) & 0xC0U) == 0x80U) {
            --length;
        }
        _bytes = {};
        std::copy_n(text.begin(), length, _bytes.begin());
        _length = static_cast<std::uint8_t>(length);
    }

    /// @brief Compares the texts.
    friend constexpr bool operator==(const String& left, const String& right) noexcept {
        return left.view() == right.view();
    }

  private:
    std::uint8_t _length{0};               ///< The number of bytes of the text.
    std::array<char, kCapacity> _bytes{};  ///< The text, then zeros.
};

static_assert(sizeof(String) == 256 && alignof(String) == 1, "String must match FieldType::kString");
static_assert(std::is_trivially_copyable_v<String>, "String must stay plain data");

/// @brief A String field is reflected as kString.
template <>
struct FieldTypeOf<String> : std::integral_constant<FieldType, FieldType::kString> {};
}  // namespace rtype::ecs
