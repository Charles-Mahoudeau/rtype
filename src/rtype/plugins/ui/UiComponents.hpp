/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UiComponents
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <string>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"   // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"       // IWYU pragma: keep
#include "plugins/GraphicsReflection.hpp"  // IWYU pragma: keep
#include "plugins/text/TextRenderer.hpp"   // IWYU pragma: keep
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::ui {

/// @brief Where the layout put a node, for this frame: written by the UiPlugin, read by anything that needs it.
struct UiComputed {
    glm::vec2 position{0.0F};     ///< Its top-left corner, in view pixels.
    glm::vec2 size{0.0F};         ///< Its size, in view pixels.
    glm::vec2 contentSize{0.0F};  ///< What its children take, from its top-left corner, before any scroll.
    glm::vec4 clip{0.0F};         ///< What it is visible in: x, y, width, height, in view pixels.
    glm::vec4 padding{0.0F};      ///< Its padding, in view pixels: left, top, right, bottom.
    std::uint32_t order = 0;      ///< Its rank in the draw order, from 0 at the bottom.
};
RTYPE_ECS_COMPONENT(UiComputed, "rtype.UiComputed", position, size, contentSize, clip, padding, order);

/// @brief Fills a node with a colour, rounded and outlined if asked: the renderer's signed distance shapes.
struct UiBackground {
    contract::renderer::Color color{1.0F};        ///< Its fill; fully transparent: none.
    float cornerRadius = 0.0F;                    ///< In view pixels.
    contract::renderer::Color borderColor{0.0F};  ///< Its outline.
    float borderWidth = 0.0F;                     ///< In view pixels, inside the node. 0: no outline.
};
RTYPE_ECS_COMPONENT(UiBackground, "rtype.UiBackground", color, cornerRadius, borderColor, borderWidth);

/// @brief Fills a node with an image, stretched, or by nine parts if `slice` is set.
struct UiImage {
    core::asset::AssetId texture;           ///< From TextureAssets::load().
    contract::renderer::math::Rect source;  ///< Part of the texture; empty: all of it.
    contract::renderer::Color color{1.0F};  ///< Multiplies the texture.
    glm::vec4 slice{0.0F};    ///< 9-slice borders of the source, left, top, right, bottom, in pixels: they keep their
                              ///< size, and only the middle stretches. All 0: stretched as a whole.
    float sliceScale = 1.0F;  ///< View pixels per pixel of the borders.
};
RTYPE_ECS_COMPONENT(UiImage, "rtype.UiImage", texture, source, color, slice, sliceScale);

/// @brief Writes in a node, which is as big as its text unless its UiNode says otherwise.
struct UiText {
    std::string text;                                ///< UTF-8; `\n` starts a new line.
    core::asset::AssetId font;                       ///< From FontAssets::load().
    float size = 24.0F;                              ///< The font size, in view pixels.
    contract::renderer::Color color{1.0F};           ///< Fills the glyphs.
    text::TextAlign align = text::TextAlign::kLeft;  ///< How the lines line up in the node.
    bool wrap = false;                               ///< Breaks the lines between words to stay inside the node.
};
RTYPE_ECS_COMPONENT(UiText, "rtype.UiText", text, font, size, color, align, wrap);

}  // namespace rtype::plugins::ui
