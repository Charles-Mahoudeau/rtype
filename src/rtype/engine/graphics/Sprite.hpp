/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sprite
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "engine/graphics/Color.hpp"
#include "engine/graphics/Texture.hpp"
#include "engine/math/Rect.hpp"

namespace rtype::engine::graphics {

/// @brief A textured rectangle, drawn with IRenderer::draw(). A plain value: build one per draw, or keep it in a
/// component.
struct Sprite {
    TextureId texture;         ///< Not owned: the Texture must outlive the frame the sprite is drawn in.
    math::Rect source;         ///< Part of the texture to show, in pixels (a frame of a sprite sheet).
    glm::vec2 position{0.0F};  ///< In world units.
    glm::vec2 scale{1.0F};     ///< 1: one world unit per texture pixel.
    glm::vec2 origin{0.0F};    ///< Pivot of the position and the rotation, in pixels from the top-left of source.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    Color color{1.0F};         ///< Multiplies the texture: tint and transparency.
};
}  // namespace rtype::engine::graphics
