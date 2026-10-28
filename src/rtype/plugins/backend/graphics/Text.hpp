/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Text
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <vector>

#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief One glyph of a Text, already laid out.
struct GlyphQuad {
    math::Rect source;  ///< Where the glyph is in the font atlas, in pixels.
    math::Rect bounds;  ///< Where it is drawn, in text units (scaled by Text::scale), from the text's top-left corner.
};

/// @brief A line or a block of text laid out by the text plugin, drawn with IRenderer::draw(): its glyphs, cut from a
/// multi-channel signed distance field (MSDF) font atlas, so it stays sharp at any size.
/// @details The atlas stores, in its red, green and blue channels, three distances to the glyph's edges; the renderer
/// draws a pixel where their median is past the middle, smoothed over about one screen pixel. The glyphs are placed in
/// text units from the top-left corner of the text, which Text::scale turns into world units, turned by
/// Text::rotation around Text::origin and placed at Text::position, as a Sprite is.
/// @code
/// renderer.draw(Text{.texture = atlas, .glyphs = layout.glyphs, .position = {400.0F, 300.0F},
///                    .origin = layout.size / 2.0F, .distanceRange = 4.0F});
/// @endcode
struct Text {
    TextureId texture;                    ///< The font atlas: an MSDF image, created with TextureEncoding::kLinear.
    std::vector<GlyphQuad> glyphs;        ///< In draw order.
    glm::vec2 position{0.0F};             ///< In world units.
    glm::vec2 scale{1.0F};                ///< Text units to world units.
    glm::vec2 origin{0.0F};               ///< Pivot of the position and the rotation, in text units from the top-left.
    float rotation = 0.0F;                ///< In radians, clockwise on screen.
    Color color{1.0F};                    ///< Fills the glyphs: colour and transparency.
    float distanceRange = 4.0F;           ///< The atlas' distance range, in atlas pixels (msdf-atlas-gen's -pxrange).
    BlendMode blend = BlendMode::kAlpha;  ///< kAlpha or kAdditive (kCutout is drawn as kAlpha).
};

}  // namespace rtype::plugins::backend::graphics
