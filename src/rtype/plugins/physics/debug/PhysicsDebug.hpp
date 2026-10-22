/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsDebug
*/

#pragma once

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "plugins/backend/graphics/Color.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::physics::debug {

/// @brief Draws an entity's physics on the client: add it to see its collider and its speed, as gizmos in Unity.
/// @details Read by the PhysicsDebugPlugin, which does all the drawing. A separate component rather than a field of
/// Collider or RigidBody: changing those rebuilds the body and restarts its contacts, while adding or removing this one
/// never touches the simulation.
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {100.0F, 200.0F}})
///     .add(rtype::plugins::physics::Collider{.shape = rtype::plugins::physics::ShapeType::kCircle, .radius = 8.0F})
///     .add(rtype::plugins::physics::RigidBody{})
///     .add(rtype::plugins::physics::debug::PhysicsDebug{});
/// @endcode
struct PhysicsDebug {
    bool shape = true;                                       ///< Draws the Collider's outline.
    bool velocity = true;                                    ///< Draws the Velocity as an arrow.
    backend::graphics::Color color{0.2F, 1.0F, 0.4F, 1.0F};  ///< The outline's and arrow's colour.
};
RTYPE_ECS_COMPONENT(PhysicsDebug, "rtype.PhysicsDebug", shape, velocity, color);

}  // namespace rtype::plugins::physics::debug
