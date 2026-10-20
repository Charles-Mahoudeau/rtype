/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerFixtures
*/

#pragma once

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::test::server {
/// @brief A replicated component of the game: two floats.
struct Position {
    float x{0.F};
    float y{0.F};
};
RTYPE_ECS_COMPONENT(Position, "test.server.Position", x, y);

/// @brief A replicated component of the game with an entity field.
struct Follow {
    rtype::ecs::Entity target;
};
RTYPE_ECS_COMPONENT(Follow, "test.server.Follow", target);

/// @brief A component the server does not replicate.
struct Secret {
    float value{0.F};
};
RTYPE_ECS_COMPONENT(Secret, "test.server.Secret", value);
}  // namespace rtype::test::server
