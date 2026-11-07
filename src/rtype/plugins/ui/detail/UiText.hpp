/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiText
*/

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "plugins/text/Font.hpp"
#include "plugins/ui/Export.hpp"

namespace rtype::plugins::ui::detail {

/// @return @p text with new lines between its words, so that no line is wider than @p width at @p size, as far as
/// it can: a word wider than @p width stays whole on its line.
[[nodiscard]] RTYPE_PLUGIN_UI_API std::string wrapText(const text::Font& font, std::string_view text, float size,
                                                       float width);

/// @return @p codepoint in UTF-8.
[[nodiscard]] RTYPE_PLUGIN_UI_API std::string encodeUtf8(char32_t codepoint);

/// @return The byte before @p caret where the UTF-8 character ending there starts; 0 at the start.
[[nodiscard]] RTYPE_PLUGIN_UI_API std::size_t previousCharacter(std::string_view text, std::size_t caret);

/// @return The byte after the UTF-8 character starting at @p caret; the size of @p text at its end.
[[nodiscard]] RTYPE_PLUGIN_UI_API std::size_t nextCharacter(std::string_view text, std::size_t caret);

}  // namespace rtype::plugins::ui::detail
