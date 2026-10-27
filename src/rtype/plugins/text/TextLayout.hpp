/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextLayout
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <string_view>
#include <vector>

#include "plugins/backend/graphics/Text.hpp"
#include "plugins/text/Font.hpp"

namespace rtype::plugins::text {

/// @brief How the lines of a text line up with each other.
enum class TextAlign : std::uint8_t {
    kLeft,    ///< On their left edge.
    kCenter,  ///< On their middle.
    kRight,   ///< On their right edge.
};

/// @brief A text placed glyph by glyph: what layoutText() returns and graphics::Text draws.
struct TextLayout {
    std::vector<backend::graphics::GlyphQuad> glyphs;  ///< The visible glyphs, from the top-left corner of the text.
    glm::vec2 size{0.0F};  ///< The width of the widest line, and the height of every line, in text units.
};

/// @brief Places the glyphs of the UTF-8 @p text, @p size text units high per em (the font size), each line under
/// the previous one.
/// @details The pen moves by each glyph's advance plus the kerning of the pair; `\n` starts a new line, `\r` is
/// ignored. A character the font does not have is drawn as `?`, or skipped if the font has no `?` either. The first
/// baseline sits the font's ascender below the top. Blank glyphs (spaces) move the pen but get no quad.
/// @return The glyphs, with the size of the text: lines * line height high.
[[nodiscard]] TextLayout layoutText(const Font& font, std::string_view text, float size,
                                    TextAlign align = TextAlign::kLeft);

/// @return The size layoutText() gives @p text, without placing its glyphs: to centre or align it beforehand.
[[nodiscard]] glm::vec2 measureText(const Font& font, std::string_view text, float size);

}  // namespace rtype::plugins::text
