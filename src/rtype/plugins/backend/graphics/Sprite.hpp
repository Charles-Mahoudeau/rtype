/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sprite
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief A textured rectangle, drawn with IRenderer::draw(). A plain value: build one per draw, or keep it in a
/// component.
struct Sprite {
    TextureId texture;         ///< Not owned: the texture must not be destroyed before the frame is shown.
    math::Rect source;         ///< Part of the texture to show, in pixels (a frame of a sprite sheet).
    glm::vec2 position{0.0F};  ///< In world units.
    glm::vec2 scale{1.0F};     ///< 1: one world unit per texture pixel.
    glm::vec2 origin{0.0F};    ///< Pivot of the position and the rotation, in pixels from the top-left of source.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    Color color{1.0F};         ///< Multiplies the texture: tint and transparency.
    BlendMode blend = BlendMode::kAlpha;  ///< How it is combined with what is under it.
};
}  // namespace rtype::plugins::backend::graphics
