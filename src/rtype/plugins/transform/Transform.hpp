/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Transform
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::transform {

/// @brief Where an entity is: what gameplay writes. Relative to the world until entities have parents (hierarchy).
/// @code
/// commands.spawn().add(rtype::plugins::transform::Transform{.position = {400.0F, 300.0F}});
/// @endcode
struct Transform {
    glm::vec2 position{0.0F};  ///< In world units, X right, Y down.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    glm::vec2 scale{1.0F};     ///< 1: natural size; negative flips.
};
RTYPE_ECS_COMPONENT(Transform, "rtype.Transform", position, rotation, scale);

/// @brief Where an entity is in the world: what rendering and collisions read, never written by gameplay.
/// @details Computed from Transform in PostUpdate by the TransformPlugin, which adds it to every entity with a
/// Transform. Equal to Transform until entities have parents; then it will combine the parents' transforms. Not
/// replicated: each process computes it.
struct GlobalTransform {
    glm::vec2 position{0.0F};  ///< In world units.
    float rotation = 0.0F;     ///< In radians, clockwise on screen.
    glm::vec2 scale{1.0F};     ///< Combined scale.
};
RTYPE_ECS_COMPONENT(GlobalTransform, "rtype.GlobalTransform", position, rotation, scale);

}  // namespace rtype::plugins::transform
