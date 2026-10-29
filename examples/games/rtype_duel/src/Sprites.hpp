/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Sprites
*/

#pragma once

#include "Components.hpp"
#include "Resources.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/sprite/AnimationEvents.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAnimator.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

// Client systems: what the server replicates, dressed with sprites, animations and explosions.

namespace rtype_duel {

/// @brief Startup: builds the atlases of the ships, the lasers and the explosion, cut from the sheets of assets/.
void loadAtlases(rtype::ecs::ResMut<rtype::plugins::sprite::AtlasAssets> atlases, rtype::ecs::ResMut<GameAtlases> game);

/// @brief Update: gives the ships and lasers the server created a Transform, a sprite and an animation.
void dressEntities(rtype::ecs::Commands& commands, rtype::ecs::Res<GameAtlases> game,
                   rtype::ecs::Query<const Ship, rtype::ecs::Without<rtype::plugins::backend::SpriteRenderer>> ships,
                   rtype::ecs::Query<const Laser, rtype::ecs::Without<rtype::plugins::backend::SpriteRenderer>> lasers);

/// @brief Update: places the obstacles the server created, which the physics debug view and drawObstacles() read.
void placeObstacles(
    rtype::ecs::Commands& commands,
    rtype::ecs::Query<const Obstacle, rtype::ecs::Without<rtype::plugins::transform::Transform>> obstacles);

/// @brief Render, after the sprites: draws each obstacle as its collider's shape, filled.
void drawObstacles(rtype::ecs::NonSendMut<rtype::plugins::backend::Backend> backend,
                   rtype::ecs::Query<const rtype::plugins::physics::Collider,
                                     const rtype::plugins::transform::GlobalTransform, rtype::ecs::With<Obstacle>>
                       obstacles);

/// @brief Update: places each ship's sprite where the server says the ship is.
void followShips(rtype::ecs::Query<rtype::plugins::transform::Transform, const Ship> ships);

/// @brief Update: places each laser's sprite where the server says the laser is.
void followLasers(rtype::ecs::Query<rtype::plugins::transform::Transform, const Laser> lasers);

/// @brief Update: when a laser wins the game, blows up the ship it hit (the other player's): hides its sprite and plays
/// an explosion where it was.
void explodeLoser(rtype::ecs::Commands& commands, rtype::ecs::Res<GameAtlases> game,
                  rtype::ecs::Query<const Winner, rtype::ecs::Added<Winner>> winners,
                  rtype::ecs::Query<const Ship, rtype::plugins::backend::SpriteRenderer> ships);

/// @brief Update: removes the explosions whose animation ended.
void removeExplosions(
    rtype::ecs::Commands& commands, rtype::ecs::EventReader<rtype::plugins::sprite::AnimationFinished> finished,
    rtype::ecs::Query<const rtype::plugins::sprite::SpriteAnimator, rtype::ecs::With<Explosion>> explosions);

}  // namespace rtype_duel
