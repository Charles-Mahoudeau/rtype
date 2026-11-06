/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Children
*/

#pragma once

#include <vector>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::ecs {
/// @brief This entity's children, in the order they were attached. Kept by the World: read it, never write it.
/// Not replicated: each side rebuilds it from Parent.
struct Children {
    std::vector<Entity> entities;  ///< The children.
};
RTYPE_ECS_COMPONENT(Children, "rtype.Children", entities);
}  // namespace rtype::ecs
