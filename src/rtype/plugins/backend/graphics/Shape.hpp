/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Shape
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>

#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Color.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief Which geometric shape a Shape is.
enum class ShapeKind : std::uint8_t {
    kRect,     ///< A rectangle of Shape::size, its corners rounded by Shape::cornerRadius.
    kCircle,   ///< A circle of Shape::radius.
    kCapsule,  ///< A capsule along its local Y axis: Shape::radius, and Shape::halfHeight for its straight part.
    kLine,     ///< A segment from Shape::position to Shape::end, Shape::radius thick on each side, round-capped.
};

/// @brief A plain coloured shape, filled and/or outlined, drawn with IRenderer::draw(): hitboxes, debug views, bars,
/// UI panels.
/// @details Each kind reads its own fields. Rectangles, circles and capsules are centred on position and turn around
/// it; a line runs from position to end. The outline is drawn inside the shape, so its outer size stays the shape's.
/// @code
/// renderer.draw(Shape{.kind = ShapeKind::kRect, .position = {400.0F, 300.0F}, .size = {64.0F, 32.0F},
///                     .cornerRadius = 4.0F, .fillColor = {0.2F, 0.2F, 0.2F, 0.8F}});
/// renderer.draw(Shape{.kind = ShapeKind::kCircle, .position = {100.0F, 100.0F}, .radius = 16.0F,
///                     .fillColor = {0.0F}, .outlineColor = {0.0F, 1.0F, 0.0F, 1.0F}, .outlineThickness = 1.0F});
/// @endcode
struct Shape {
    ShapeKind kind = ShapeKind::kRect;    ///< Which fields below describe the shape.
    glm::vec2 position{0.0F};             ///< The center (rect, circle, capsule) or the start (line), in world units.
    glm::vec2 end{0.0F};                  ///< kLine: the end, in world units.
    glm::vec2 size{0.0F};                 ///< kRect: full width and height, in world units.
    float radius = 0.0F;                  ///< kCircle, kCapsule: radius; kLine: half the thickness. World units.
    float halfHeight = 0.0F;              ///< kCapsule: half the length of the straight part, in world units.
    float cornerRadius = 0.0F;            ///< kRect: radius of the rounded corners, in world units; 0: sharp.
    float rotation = 0.0F;                ///< kRect, kCapsule: in radians, clockwise on screen, around position.
    Color fillColor{1.0F};                ///< Fully transparent: outline only.
    Color outlineColor{0.0F};             ///< The outline's colour.
    float outlineThickness = 0.0F;        ///< In world units, drawn inside the shape. 0: no outline.
    BlendMode blend = BlendMode::kAlpha;  ///< kAlpha or kAdditive (kCutout is drawn as kAlpha).
};

}  // namespace rtype::plugins::backend::graphics
