/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Background
*/

#pragma once

#include "Components.hpp"
#include "Resources.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/tilemap/Parallax.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

// Client systems: the space background and the planets that pass behind the arena.

namespace rtype_duel {

/// @brief Startup: spawns the space background, its layers scrolling left at their own speed behind the arena.
void spawnBackground(rtype::ecs::ResMut<rtype::plugins::tilemap::TilemapAssets> maps, rtype::ecs::Commands& commands);

/// @brief Startup: asks for the images of the planets that pass behind the arena; the first one comes soon after.
void loadPlanets(rtype::ecs::ResMut<rtype::plugins::backend::TextureAssets> textures,
                 rtype::ecs::ResMut<BackgroundPlanets> planets);

/// @brief Update: now and then, sends a random planet across the background at a random height and distance, scrolling
/// left a little faster than the background's layers.
void spawnPlanets(rtype::ecs::Commands& commands, rtype::ecs::ResMut<Randomness> randomness,
                  rtype::ecs::ResMut<BackgroundPlanets> planets, rtype::ecs::Res<rtype::ecs::Time> time);

/// @brief Update: removes the planets that have left the view on the left.
void removePlanets(rtype::ecs::Commands& commands,
                   rtype::ecs::Query<const rtype::plugins::transform::Transform,
                                     const rtype::plugins::tilemap::Parallax, const BackgroundPlanet>
                       planets);

}  // namespace rtype_duel
