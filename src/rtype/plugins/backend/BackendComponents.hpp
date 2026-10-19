/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendComponents
*/

#pragma once

#include "plugins/backend/graphics/Camera.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

// The backend types used as components. The declaring macro must sit in the namespace of the type, so this reopens
// the graphics one; Camera.hpp itself stays free of the ECS, since the renderers include it.
namespace rtype::plugins::backend::graphics {
/// @brief The camera of an entity: the backend plugin shows the one with the highest priority. Its fields are not
/// reflected: the projection is a variant, which reflection does not describe.
}  // namespace rtype::plugins::backend::graphics
