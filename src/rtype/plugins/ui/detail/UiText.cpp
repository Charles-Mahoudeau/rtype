/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiText
*/

#include "plugins/ui/detail/UiText.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "plugins/text/Font.hpp"
#include "plugins/text/TextLayout.hpp"

namespace rtype::plugins::ui::detail {
namespace {
/// @return Whether @p byte continues a UTF-8 character.
[[nodiscard]] bool isContinuation(const char byte) noexcept {
    return (static_cast<std::uint8_t>(byte) & 0xC0U) == 0x80U;
}

/// @brief Appends the paragraph @p paragraph to @p result, wrapped.
void wrapParagraph(const text::Font& font, const std::string_view paragraph, const float size, const float width,
                   std::string& result) {
    std::string line;
    std::size_t start = 0;
    while (start <= paragraph.size()) {
        const std::size_t end = paragraph.find(' ', start);
        const std::string_view word = paragraph.substr(start, end == std::string_view::npos ? end : end - start);
        std::string candidate = line;
        if (!candidate.empty()) {
            candidate += ' ';
        }
        candidate += word;
        if (!line.empty() && text::measureText(font, candidate, size).x > width) {
            result += line;
            result += '\n';
            line = word;
        } else {
            line = std::move(candidate);
        }
        if (end == std::string_view::npos) {
            break;
        }
        start = end + 1;
    }
    result += line;
}
}  // namespace

std::string wrapText(const text::Font& font, const std::string_view text, const float size, const float width) {
    std::string result;
    std::size_t start = 0;
    while (true) {
        const std::size_t end = text.find('\n', start);
        wrapParagraph(font, text.substr(start, end == std::string_view::npos ? end : end - start), size, width, result);
        if (end == std::string_view::npos) {
            return result;
        }
        result += '\n';
        start = end + 1;
    }
}

std::string encodeUtf8(const char32_t codepoint) {
    const auto value = static_cast<std::uint32_t>(codepoint);
    std::string bytes;
    if (value < 0x80U) {
        bytes += static_cast<char>(value);
    } else if (value < 0x800U) {
        bytes += static_cast<char>(0xC0U | (value >> 6U));
        bytes += static_cast<char>(0x80U | (value & 0x3FU));
    } else if (value < 0x10000U) {
        bytes += static_cast<char>(0xE0U | (value >> 12U));
        bytes += static_cast<char>(0x80U | ((value >> 6U) & 0x3FU));
        bytes += static_cast<char>(0x80U | (value & 0x3FU));
    } else {
        bytes += static_cast<char>(0xF0U | (value >> 18U));
        bytes += static_cast<char>(0x80U | ((value >> 12U) & 0x3FU));
        bytes += static_cast<char>(0x80U | ((value >> 6U) & 0x3FU));
        bytes += static_cast<char>(0x80U | (value & 0x3FU));
    }
    return bytes;
}

std::size_t previousCharacter(const std::string_view text, std::size_t caret) noexcept {
    if (caret == 0 || caret > text.size()) {
        return caret == 0 ? 0 : text.size();
    }
    --caret;
    while (caret > 0 && isContinuation(text[caret])) {
        --caret;
    }
    return caret;
}

std::size_t nextCharacter(const std::string_view text, std::size_t caret) noexcept {
    if (caret >= text.size()) {
        return text.size();
    }
    ++caret;
    while (caret < text.size() && isContinuation(text[caret])) {
        ++caret;
    }
    return caret;
}
}  // namespace rtype::plugins::ui::detail
