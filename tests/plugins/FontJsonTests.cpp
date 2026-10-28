/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FontJsonTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_uint2.hpp>
#include <string_view>

#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/text/Font.hpp"
#include "plugins/text/detail/FontJson.hpp"
#include "plugins/text/exceptions/TextExceptions.hpp"

using rtype::plugins::text::Font;
using rtype::plugins::text::Glyph;
using rtype::plugins::text::detail::parseFontJson;

namespace {
/// @brief A font of a space, an 'A' and a kerning pair, as msdf-atlas-gen writes it with -yorigin top.
constexpr std::string_view kFontTop = R"({
  "atlas": { "type": "msdf", "distanceRange": 4, "size": 48, "width": 256, "height": 128, "yOrigin": "top" },
  "metrics": { "emSize": 1, "lineHeight": 1.25, "ascender": -1, "descender": 0.25 },
  "glyphs": [
    { "unicode": 32, "advance": 0.25 },
    { "unicode": 65, "advance": 0.75,
      "planeBounds": { "left": 0.0, "top": -0.75, "right": 0.5, "bottom": 0.0 },
      "atlasBounds": { "left": 10.5, "top": 20.5, "right": 34.5, "bottom": 56.5 } }
  ],
  "kerning": [ { "unicode1": 65, "unicode2": 65, "advance": -0.125 } ]
})";

/// @brief The same 'A', written Y up (msdf-atlas-gen's default, -yorigin bottom).
constexpr std::string_view kFontBottom = R"({
  "atlas": { "type": "mtsdf", "distanceRange": 2, "size": 48, "width": 256, "height": 128, "yOrigin": "bottom" },
  "metrics": { "emSize": 1, "lineHeight": 1.25, "ascender": 1, "descender": -0.25 },
  "glyphs": [
    { "unicode": 65, "advance": 0.75,
      "planeBounds": { "left": 0.0, "bottom": 0.0, "right": 0.5, "top": 0.75 },
      "atlasBounds": { "left": 10.5, "bottom": 71.5, "right": 34.5, "top": 107.5 } }
  ]
})";

/// @brief Expects @p glyph to be the 'A' of both files: Y down.
void expectA(const Glyph& glyph) {
    EXPECT_FLOAT_EQ(glyph.advance, 0.75F);
    EXPECT_FLOAT_EQ(glyph.plane.getX(), 0.0F);
    EXPECT_FLOAT_EQ(glyph.plane.getY(), -0.75F);
    EXPECT_FLOAT_EQ(glyph.plane.getWidth(), 0.5F);
    EXPECT_FLOAT_EQ(glyph.plane.getHeight(), 0.75F);
    EXPECT_FLOAT_EQ(glyph.atlas.getX(), 10.5F);
    EXPECT_FLOAT_EQ(glyph.atlas.getY(), 20.5F);
    EXPECT_FLOAT_EQ(glyph.atlas.getWidth(), 24.0F);
    EXPECT_FLOAT_EQ(glyph.atlas.getHeight(), 36.0F);
}
}  // namespace

TEST(FontJson, ReadsTheAtlasAndTheMetrics) {
    const Font font = parseFontJson(kFontTop, "assets/fonts/test.json");
    EXPECT_EQ(font.getTexturePath(), "assets/fonts/test.png");
    EXPECT_EQ(font.getTexture(), rtype::plugins::asset::AssetId::fromPath("assets/fonts/test.png"));
    EXPECT_EQ(font.getTextureDesc().size, (glm::uvec2{256, 128}));
    EXPECT_EQ(font.getTextureDesc().filter, rtype::plugins::backend::graphics::TextureFilter::kLinear);
    EXPECT_EQ(font.getTextureDesc().encoding, rtype::plugins::backend::graphics::TextureEncoding::kLinear);
    EXPECT_FLOAT_EQ(font.getDistanceRange(), 4.0F);
    EXPECT_FLOAT_EQ(font.getMetrics().lineHeight, 1.25F);
    EXPECT_FLOAT_EQ(font.getMetrics().ascender, -1.0F);
    EXPECT_FLOAT_EQ(font.getMetrics().descender, 0.25F);
}

TEST(FontJson, ReadsTheGlyphsAndTheKerning) {
    const Font font = parseFontJson(kFontTop, "test.json");
    EXPECT_EQ(font.getGlyphCount(), 2U);
    const Glyph* space = font.findGlyph(U' ');
    ASSERT_NE(space, nullptr);
    EXPECT_TRUE(space->isBlank());
    EXPECT_FLOAT_EQ(space->advance, 0.25F);
    const Glyph* a = font.findGlyph(U'A');
    ASSERT_NE(a, nullptr);
    expectA(*a);
    EXPECT_FLOAT_EQ(font.getKerning(U'A', U'A'), -0.125F);
    EXPECT_FLOAT_EQ(font.getKerning(U'A', U' '), 0.0F);
    EXPECT_EQ(font.findGlyph(U'B'), nullptr);
}

TEST(FontJson, FlipsAFileWrittenYUp) {
    const Font font = parseFontJson(kFontBottom, "test.json");
    EXPECT_FLOAT_EQ(font.getMetrics().ascender, -1.0F);
    EXPECT_FLOAT_EQ(font.getMetrics().descender, 0.25F);
    const Glyph* a = font.findGlyph(U'A');
    ASSERT_NE(a, nullptr);
    expectA(*a);
}

TEST(FontJson, RejectsAnAtlasThatIsNotMsdf) {
    constexpr std::string_view kHardMask = R"({ "atlas": { "type": "hardmask", "distanceRange": 0,
        "width": 1, "height": 1 }, "metrics": {}, "glyphs": [] })";
    EXPECT_THROW(static_cast<void>(parseFontJson(kHardMask, "mask.json")),
                 rtype::plugins::text::exceptions::FontException);
}

TEST(FontJson, NamesTheFileOfABrokenFont) {
    try {
        static_cast<void>(parseFontJson("{ not json", "broken.json"));
        FAIL() << "expected a FontException";
    } catch (const rtype::plugins::text::exceptions::FontException& error) {
        EXPECT_NE(std::string_view{error.what()}.find("broken.json"), std::string_view::npos);
    }
}
