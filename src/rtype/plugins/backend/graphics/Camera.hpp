/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Camera
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float3.hpp>

#include "plugins/backend/graphics/View.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief The point of view the world is seen from, given to IRenderer::setView() as a View.
/// @details In an App it is also a component ("rtype.Camera", declared below): the backend plugin applies the camera of
/// the entity that has the highest priority.
struct Camera {
    glm::vec3 position{0.0F};                ///< World point at the centre of the screen. Z is unused in 2D.
    float rotation = 0.0F;                   ///< Around the view axis, in radians, clockwise on screen.
    Projection projection = Orthographic{};  ///< How the world is projected.
    std::int32_t priority = 0;               ///< Among several camera entities, the highest one is shown.
};

RTYPE_ECS_COMPONENT(Camera, "rtype.Camera");
}  // namespace rtype::plugins::backend::graphics
