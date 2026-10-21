/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsDebugShapes
*/

#include "plugins/physics/debug/PhysicsDebugShapes.hpp"

#include <algorithm>
#include <cmath>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <vector>

#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/transform/Transform.hpp"

namespace rtype::plugins::physics::debug {
namespace {
constexpr float kMinArrowLength = 0.5F;  ///< Shorter arrows are not drawn: a still body has none.
constexpr float kHeadLength = 8.0F;      ///< The arrow head's strokes, at most, in world units.
constexpr float kHeadShare = 0.3F;       ///< ...and at most this share of the arrow.
constexpr float kHeadCos = 0.866F;       ///< cos(30 degrees): the strokes' angle with the shaft.
constexpr float kHeadSin = 0.5F;         ///< sin(30 degrees).
constexpr float kSensorOpacity = 0.5F;   ///< A sensor's outline is fainter.

/// @return @p local turned by @p rotation, clockwise on screen with Y down, as the renderer turns shapes.
glm::vec2 rotate(glm::vec2 local, float rotation) noexcept {
    const float c = std::cos(rotation);
    const float s = std::sin(rotation);
    return {(c * local.x) - (s * local.y), (s * local.x) + (c * local.y)};
}
}  // namespace

backend::graphics::Shape debugColliderShape(const Collider& collider, const transform::GlobalTransform& transform,
                                            backend::graphics::Color color, float thickness) noexcept {
    using backend::graphics::ShapeKind;
    if (collider.sensor) {
        color.w *= kSensorOpacity;
    }
    backend::graphics::Shape shape{
        .position = transform.position + rotate(collider.offset, transform.rotation),
        .rotation = transform.rotation,
        .fillColor = backend::graphics::Color{0.0F},
        .outlineColor = color,
        .outlineThickness = thickness,
    };
    switch (collider.shape) {
        case ShapeType::kBox:
            shape.kind = ShapeKind::kRect;
            shape.size = collider.size;
            break;
        case ShapeType::kCircle:
            shape.kind = ShapeKind::kCircle;
            shape.radius = collider.radius;
            break;
        case ShapeType::kCapsule:
            shape.kind = ShapeKind::kCapsule;
            shape.radius = collider.radius;
            shape.halfHeight = collider.halfHeight;
            break;
    }
    return shape;
}

std::vector<backend::graphics::Shape> debugVelocityArrow(glm::vec2 from, glm::vec2 velocity, float scale,
                                                         backend::graphics::Color color, float thickness) {
    using backend::graphics::Shape;
    using backend::graphics::ShapeKind;
    const glm::vec2 travel = velocity * scale;
    const float length = glm::length(travel);
    if (!(length >= kMinArrowLength)) {
        return {};
    }
    const glm::vec2 to = from + travel;
    const glm::vec2 direction = travel / length;
    const glm::vec2 side{-direction.y, direction.x};
    const float head = std::min(kHeadLength, length * kHeadShare);
    const auto stroke = [&](glm::vec2 start, glm::vec2 end) {
        return Shape{
            .kind = ShapeKind::kLine, .position = start, .end = end, .radius = thickness * 0.5F, .fillColor = color};
    };
    return {
        stroke(from, to),
        stroke(to, to - (((direction * kHeadCos) + (side * kHeadSin)) * head)),
        stroke(to, to - (((direction * kHeadCos) - (side * kHeadSin)) * head)),
    };
}

}  // namespace rtype::plugins::physics::debug
