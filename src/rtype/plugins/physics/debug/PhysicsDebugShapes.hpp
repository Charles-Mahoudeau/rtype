/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsDebugShapes
*/

#pragma once

#include <glm/ext/vector_float2.hpp>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/debug/Export.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Shape.hpp"

namespace rtype::plugins::physics::debug {

/// @return The outline of a collider where its entity is: a rectangle, circle or capsule turned with the entity and
/// moved by the collider's offset, in world units. A sensor is drawn at half the colour's opacity.
[[nodiscard]] RTYPE_PLUGIN_PHYSICS_DEBUG_API contract::renderer::Shape debugColliderShape(
    const Collider& collider, const transform::GlobalTransform& transform, contract::renderer::Color color,
    float thickness) noexcept;

/// @return An arrow from @p from along @p velocity, as long as the distance covered in @p scale seconds: its shaft and
/// the two strokes of its head, as lines. Nothing for a speed too small to draw.
[[nodiscard]] RTYPE_PLUGIN_PHYSICS_DEBUG_API std::vector<contract::renderer::Shape> debugVelocityArrow(
    glm::vec2 from, glm::vec2 velocity, float scale, contract::renderer::Color color, float thickness);

}  // namespace rtype::plugins::physics::debug
