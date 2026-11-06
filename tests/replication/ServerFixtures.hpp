/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerFixtures
*/

#pragma once

#include <cstdint>
#include <vector>

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

/// @brief A replicated component of the game with lists: not plain data, and of a size that depends on its value.
struct Cargo {
    std::vector<std::uint32_t> items;
    std::vector<rtype::ecs::Entity> crew;
    float weight{0.F};
};
RTYPE_ECS_COMPONENT(Cargo, "test.server.Cargo", items, crew, weight);

/// @brief A component the server does not replicate.
struct Secret {
    float value{0.F};
};
RTYPE_ECS_COMPONENT(Secret, "test.server.Secret", value);
}  // namespace rtype::test::server
