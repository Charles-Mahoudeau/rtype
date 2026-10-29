/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Components
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype_duel {

/// @brief A player's ship: replicated on every change.
struct Ship {
    glm::vec2 position{0.0F};  ///< In world units (the 800x600 view).
    std::uint32_t player{0};   ///< 0 on the left, firing right; 1 on the right, firing left.
};
RTYPE_ECS_COMPONENT(Ship, "game.Ship", position, player);

/// @brief A laser in flight: replicated on every change.
struct Laser {
    glm::vec2 position{0.0F};  ///< In world units.
    std::uint32_t shooter{0};  ///< The player who fired it: it flies away from them and cannot hit them.
};
RTYPE_ECS_COMPONENT(Laser, "game.Laser", position, shooter);

/// @brief A wall in the middle of the arena, spawned by the server: replicated on every change.
struct Obstacle {
    glm::vec2 position{0.0F};  ///< In world units: the clients place it with a Transform.
};
RTYPE_ECS_COMPONENT(Obstacle, "game.Obstacle", position);

/// @brief Spawned by the server when a laser hits: the game is over.
struct Winner {
    std::uint32_t player{0};  ///< The player whose laser hit.
};
RTYPE_ECS_COMPONENT(Winner, "game.Winner", player);

/// @brief Spawned by the server instead of a laser when a shot misfires: replicated on every change, so that the
/// clients play the misfire sound where it happened. The server removes it after kMisfireLifetime.
struct Misfire {
    glm::vec2 position{0.0F};  ///< Where the laser would have left the ship, in world units.
    std::uint32_t shooter{0};  ///< The player whose shot misfired.
};
RTYPE_ECS_COMPONENT(Misfire, "game.Misfire", position, shooter);

/// @brief Server only: how long a Misfire still lives, long enough for a snapshot to carry it to the clients.
struct MisfireTimer {
    float remaining = 0.0F;  ///< Seconds.
};
RTYPE_ECS_COMPONENT(MisfireTimer, "game.MisfireTimer");

/// @brief Client only: an explosion, removed when its animation ends.
struct Explosion {};
RTYPE_ECS_COMPONENT(Explosion, "game.Explosion");

/// @brief Client only: a planet passing behind the arena, removed once it has left the view on the left.
struct BackgroundPlanet {
    float width = 0.0F;  ///< On screen, in world units.
};
RTYPE_ECS_COMPONENT(BackgroundPlanet, "game.BackgroundPlanet");

}  // namespace rtype_duel
