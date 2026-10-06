/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RectShape
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "engine/graphics/Color.hpp"

namespace rtype::engine::graphics {

/// @brief A plain coloured rectangle, with an optional outline, drawn with IRenderer::draw(). Hitboxes, bars, UI
/// panels.
struct RectShape {
    glm::vec2 position{0.0F};  ///< In world units.
    glm::vec2 size{0.0F};      ///< In world units.
    glm::vec2 origin{0.0F};    ///< Pivot of the position and the rotation, from the top-left.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    Color fillColor{1.0F};     ///< Fully transparent: outline only.
    Color outlineColor{0.0F};
    float outlineThickness = 0.0F;  ///< In world units, drawn inside the rectangle. 0: no outline.
};
}  // namespace rtype::engine::graphics
