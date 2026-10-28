/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextLayout
*/

#include "plugins/text/TextLayout.hpp"

#include <algorithm>
#include <glm/ext/vector_float2.hpp>
#include <string_view>
#include <vector>

#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/detail/Utf8.hpp"

namespace rtype::plugins::text {

namespace {
/// @brief The glyph drawn for a character the font does not have.
constexpr char32_t kFallback = U'?';

/// @brief One character to place, resolved to its glyph.
struct Placed {
    char32_t codepoint = 0;        ///< The character drawn: the fallback for a missing one.
    const Glyph* glyph = nullptr;  ///< Its metrics.
};

/// @brief The characters of one line, and its width in ems.
struct Line {
    std::vector<Placed> characters;  ///< In order.
    float width = 0.0F;              ///< Where the pen ends, in ems.
};

/// @return The lines of @p text in @p font, their characters resolved and their width measured.
std::vector<Line> splitLines(const Font& font, std::string_view text) {
    std::vector<Line> lines(1);
    const Glyph* fallback = font.findGlyph(kFallback);
    for (const char32_t codepoint : detail::decodeUtf8(text)) {
        if (codepoint == U'\n') {
            lines.emplace_back();
            continue;
        }
        if (codepoint == U'\r') {
            continue;
        }
        Placed placed{.codepoint = codepoint, .glyph = font.findGlyph(codepoint)};
        if (placed.glyph == nullptr) {
            placed = Placed{.codepoint = kFallback, .glyph = fallback};
        }
        if (placed.glyph == nullptr) {
            continue;
        }
        Line& line = lines.back();
        if (!line.characters.empty()) {
            line.width += font.getKerning(line.characters.back().codepoint, placed.codepoint);
        }
        line.width += placed.glyph->advance;
        line.characters.push_back(placed);
    }
    return lines;
}

/// @return The size of @p lines, @p size text units per em.
glm::vec2 sizeOf(const Font& font, const std::vector<Line>& lines, float size) {
    float width = 0.0F;
    for (const Line& line : lines) {
        width = std::max(width, line.width);
    }
    return glm::vec2{width, static_cast<float>(lines.size()) * font.getMetrics().lineHeight} * size;
}

/// @return Where a line of @p width starts in a text of @p textWidth aligned by @p align.
float lineStart(TextAlign align, float width, float textWidth) {
    switch (align) {
        case TextAlign::kCenter:
            return (textWidth - width) / 2.0F;
        case TextAlign::kRight:
            return textWidth - width;
        case TextAlign::kLeft:
            break;
    }
    return 0.0F;
}
}  // namespace

TextLayout layoutText(const Font& font, std::string_view text, float size, TextAlign align) {
    const std::vector<Line> lines = splitLines(font, text);
    TextLayout layout{.size = sizeOf(font, lines, size)};
    const Font::Metrics& metrics = font.getMetrics();
    float baseline = -metrics.ascender * size;
    for (const Line& line : lines) {
        float pen = lineStart(align, line.width * size, layout.size.x);
        const Placed* previous = nullptr;
        for (const Placed& placed : line.characters) {
            if (previous != nullptr) {
                pen += font.getKerning(previous->codepoint, placed.codepoint) * size;
            }
            const Glyph& glyph = *placed.glyph;
            if (!glyph.isBlank()) {
                layout.glyphs.push_back(backend::graphics::GlyphQuad{
                    .source = glyph.atlas,
                    .bounds = backend::math::Rect{
                        glm::vec2{pen + (glyph.plane.getX() * size), baseline + (glyph.plane.getY() * size)},
                        glm::vec2{glyph.plane.getWidth(), glyph.plane.getHeight()} * size}});
            }
            pen += glyph.advance * size;
            previous = &placed;
        }
        baseline += metrics.lineHeight * size;
    }
    return layout;
}

glm::vec2 measureText(const Font& font, std::string_view text, float size) {
    return sizeOf(font, splitLines(font, text), size);
}

}  // namespace rtype::plugins::text
