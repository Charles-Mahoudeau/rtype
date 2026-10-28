/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Game
*/

#pragma once

#include <cstdint>

#include "Components.hpp"
#include "plugins/physics/Collider.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/query/Query.hpp"

// Shared by the server and the clients: the manifest checks they register the same things.

namespace rtype_duel {

/// @return Whether a Winner exists: the game is over.
bool isOver(rtype::ecs::Query<const Winner>& winners);

/// @return +1 for the left player (fires right), -1 for the right one.
float directionOf(std::uint32_t player);

/// @return The collision layer of a player's ship.
std::uint32_t shipLayerOf(std::uint32_t player);

/// @return A ship's collider: blocked by the obstacles, hit by the lasers.
rtype::plugins::physics::Collider shipCollider(std::uint32_t player);

/// @return A laser's collider: it touches the obstacles and the other player's ship, never its shooter.
rtype::plugins::physics::Collider laserCollider(std::uint32_t shooter);

/// @brief Adds the physics (which replicates Collider and RigidBody) and registers the game's replicated components.
void registerGame(rtype::ecs::App& app);

/// @brief Declares the input actions both sides agree on; after the server or client plugin inserted the layout.
void declareInputs(rtype::ecs::App& app);

}  // namespace rtype_duel
