/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextLayoutTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/TextLayout.hpp"

using rtype::plugins::backend::math::Rect;
using rtype::plugins::text::Font;
using rtype::plugins::text::Glyph;
using rtype::plugins::text::TextAlign;
using rtype::plugins::text::TextLayout;

namespace {
/// @brief A font with round numbers: lines 1.5 ems apart, the baseline 1 em down; 'A' and 'B' half an em wide
/// (advance 0.5, from the top of the em to the baseline), '?' a quarter, a space a quarter, and A then B 0.25 closer.
Font makeFont(bool withFallback = true) {
    Font font{"font.png", {}, 4.0F, Font::Metrics{.lineHeight = 1.5F, .ascender = -1.0F, .descender = 0.5F}};
    font.addGlyph(
        U'A',
        Glyph{.advance = 0.5F, .plane = Rect{{0.0F, -1.0F}, {0.5F, 1.0F}}, .atlas = Rect{{0.0F, 0.0F}, {8.0F, 16.0F}}});
    font.addGlyph(
        U'B',
        Glyph{.advance = 0.5F, .plane = Rect{{0.0F, -1.0F}, {0.5F, 1.0F}}, .atlas = Rect{{8.0F, 0.0F}, {8.0F, 16.0F}}});
    font.addGlyph(U' ', Glyph{.advance = 0.25F});
    if (withFallback) {
        font.addGlyph(U'?', Glyph{.advance = 0.25F,
                                  .plane = Rect{{0.0F, -1.0F}, {0.25F, 1.0F}},
                                  .atlas = Rect{{16.0F, 0.0F}, {4.0F, 16.0F}}});
    }
    font.setKerning(U'A', U'B', -0.25F);
    return font;
}

/// @return The top-left corner of @p rect.
glm::vec2 cornerOf(const Rect& rect) { return {rect.getX(), rect.getY()}; }
}  // namespace

TEST(TextLayout, PlacesTheGlyphsOnTheFirstBaseline) {
    const TextLayout layout = layoutText(makeFont(), "AA", 10.0F);
    ASSERT_EQ(layout.glyphs.size(), 2U);
    EXPECT_EQ(cornerOf(layout.glyphs.at(0).bounds), (glm::vec2{0.0F, 0.0F}));  // The baseline 10 down, A 10 high.
    EXPECT_EQ(cornerOf(layout.glyphs.at(1).bounds), (glm::vec2{5.0F, 0.0F}));
    EXPECT_FLOAT_EQ(layout.glyphs.at(0).bounds.getWidth(), 5.0F);
    EXPECT_FLOAT_EQ(layout.glyphs.at(0).bounds.getHeight(), 10.0F);
    EXPECT_FLOAT_EQ(layout.glyphs.at(1).source.getX(), 0.0F);
    EXPECT_EQ(layout.size, (glm::vec2{10.0F, 15.0F}));
}

TEST(TextLayout, AppliesTheKerningOfEachPair) {
    const TextLayout layout = layoutText(makeFont(), "AB", 10.0F);
    ASSERT_EQ(layout.glyphs.size(), 2U);
    EXPECT_FLOAT_EQ(layout.glyphs.at(1).bounds.getX(), 2.5F);  // 5 of advance, 2.5 of kerning.
    EXPECT_FLOAT_EQ(layout.size.x, 7.5F);
}

TEST(TextLayout, MovesThePenOverASpaceWithoutAGlyph) {
    const TextLayout layout = layoutText(makeFont(), "A A", 10.0F);
    ASSERT_EQ(layout.glyphs.size(), 2U);
    EXPECT_FLOAT_EQ(layout.glyphs.at(1).bounds.getX(), 7.5F);
}

TEST(TextLayout, StartsANewLineAtEachNewline) {
    const TextLayout layout = layoutText(makeFont(), "AA\r\nA", 10.0F);
    ASSERT_EQ(layout.glyphs.size(), 3U);
    EXPECT_EQ(cornerOf(layout.glyphs.at(2).bounds), (glm::vec2{0.0F, 15.0F}));
    EXPECT_EQ(layout.size, (glm::vec2{10.0F, 30.0F}));
}

TEST(TextLayout, AlignsTheLines) {
    const TextLayout centred = layoutText(makeFont(), "AA\nA", 10.0F, TextAlign::kCenter);
    EXPECT_FLOAT_EQ(centred.glyphs.at(2).bounds.getX(), 2.5F);
    const TextLayout right = layoutText(makeFont(), "AA\nA", 10.0F, TextAlign::kRight);
    EXPECT_FLOAT_EQ(right.glyphs.at(2).bounds.getX(), 5.0F);
}

TEST(TextLayout, DrawsAMissingCharacterAsAQuestionMark) {
    const TextLayout layout = layoutText(makeFont(), "AZ", 10.0F);
    ASSERT_EQ(layout.glyphs.size(), 2U);
    EXPECT_FLOAT_EQ(layout.glyphs.at(1).source.getX(), 16.0F);
    const TextLayout skipped = layoutText(makeFont(false), "AZ", 10.0F);
    EXPECT_EQ(skipped.glyphs.size(), 1U);
}

TEST(TextLayout, MeasuresWhatItLaysOut) {
    const Font font = makeFont();
    EXPECT_EQ(measureText(font, "AB A\nA", 20.0F), layoutText(font, "AB A\nA", 20.0F).size);
    EXPECT_EQ(measureText(font, "", 20.0F), (glm::vec2{0.0F, 30.0F}));  // One empty line.
}
