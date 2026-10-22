/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Velocity
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::physics {

/// @brief How fast an entity's body moves: gameplay writes it to set the speed, the physics plugin writes it back.
/// @details Opt-in: only the entities that have one are synced. Before each step, a Velocity that gameplay changed is
/// given to the body (a dynamic or kinematic one; a static body ignores it); after the step, a dynamic body writes its
/// velocity back, so gravity, damping, impulses and collisions show up in it. A kinematic body only reads it: it moves
/// at its Velocity, except on the ticks gameplay moves its Transform, where it moves to the Transform instead.
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {0.0F, 200.0F}})
///     .add(rtype::plugins::physics::RigidBody{.type = rtype::plugins::physics::BodyType::kKinematic})
///     .add(rtype::plugins::physics::Collider{.shape = rtype::plugins::physics::ShapeType::kCircle, .radius = 4.0F})
///     .add(rtype::plugins::physics::Velocity{.linear = {300.0F, 0.0F}});
/// @endcode
struct Velocity {
    glm::vec2 linear{0.0F};  ///< In world units per second, Y down.
    float angular = 0.0F;    ///< In radians per second, clockwise on screen.

    friend bool operator==(const Velocity& first, const Velocity& second) = default;
};
RTYPE_ECS_COMPONENT(Velocity, "rtype.Velocity", linear, angular);

}  // namespace rtype::plugins::physics
