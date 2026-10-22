/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Camera
*/

#pragma once

#include <cstdint>

#include "plugins/backend/graphics/View.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::backend::graphics {

/// @brief The component ("rtype.Camera") that makes an entity a point of view: the backend plugin shows the world from
/// the GlobalTransform of the camera entity with the highest priority.
/// @details Where the camera is comes from the entity's Transform, as for any other entity: an entity with a Camera but
/// no Transform is never shown.
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {960.0F, 540.0F}})
///     .add(rtype::plugins::backend::graphics::Camera{});
/// @endcode
struct Camera {
    Projection projection = Orthographic{};  ///< How the world is projected.
    std::int32_t priority = 0;               ///< Among several camera entities, the highest one is shown.
};

RTYPE_ECS_COMPONENT(Camera, "rtype.Camera");
}  // namespace rtype::plugins::backend::graphics
