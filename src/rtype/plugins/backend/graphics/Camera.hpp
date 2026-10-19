/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Camera
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <variant>

#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief Parallel projection, without perspective: what every 2D view is.
struct Orthographic {
    float zoom = 1.0F;  ///< Screen pixels per world unit: 2 shows everything twice as big.
};

/// @brief How the camera projects the world onto the screen.
/// @note Only Orthographic for now. A Perspective alternative will join the variant if 3D comes, without changing
/// Camera; until then, every backend (2D-only libraries included) can honour every projection.
using Projection = std::variant<Orthographic>;

/// @brief The point of view the world is seen from, set with IRenderer::setCamera().
/// @details In an App it is also a component ("rtype.Camera", declared in plugins/backend/BackendComponents.hpp): the
/// backend plugin applies the camera of the entity that has the highest priority.
struct Camera {
    glm::vec3 position{0.0F};                ///< World point at the centre of the screen. Z is unused in 2D.
    float rotation = 0.0F;                   ///< Around the view axis, in radians, clockwise on screen.
    Projection projection = Orthographic{};  ///< How the world is projected.
    std::int32_t priority = 0;               ///< Among several camera entities, the highest one is shown.
};

RTYPE_ECS_COMPONENT(Camera, "rtype.Camera");
}  // namespace rtype::plugins::backend::graphics
