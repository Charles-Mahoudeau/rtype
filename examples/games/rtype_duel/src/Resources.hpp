/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Resources
*/

#pragma once

#include <array>
#include <random>

#include "core/asset/AssetId.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype_duel {

/// @brief A random engine: the server rolls the misfires, the clients vary the shot sounds.
struct Randomness {
    std::mt19937 engine{std::random_device{}()};  ///< Seeded differently on every run.
};
RTYPE_ECS_RESOURCE(Randomness, "game.Randomness");

/// @brief Client only: the sounds of the game, asked for at startup.
struct GameSounds {
    rtype::core::asset::AssetId music;    ///< The background track, looping.
    rtype::core::asset::AssetId laser;    ///< A laser fired.
    rtype::core::asset::AssetId misfire;  ///< A shot that misfired.
};
RTYPE_ECS_RESOURCE(GameSounds, "game.GameSounds");

/// @brief Client only: the atlases of the game's sprites, built at startup.
struct GameAtlases {
    rtype::core::asset::AssetId leftShip;   ///< r-typesheet24.gif: animation "fly".
    rtype::core::asset::AssetId rightShip;  ///< r-typesheet26.gif: animation "fly".
    rtype::core::asset::AssetId laser;      ///< bullet.gif: animations "right" and "left".
    rtype::core::asset::AssetId explosion;  ///< r-fx-explosions.gif: animation "explode", played once.
};
RTYPE_ECS_RESOURCE(GameAtlases, "game.GameAtlases");

/// @brief Client only: the font of the game's texts, read at startup.
struct GameFonts {
    rtype::core::asset::AssetId text;  ///< assets/fonts/orbitron.json.
};
RTYPE_ECS_RESOURCE(GameFonts, "game.GameFonts");

/// @brief Client only: the images of the planets that pass behind the arena now and then, and when the next one comes.
struct BackgroundPlanets {
    std::array<rtype::core::asset::AssetId, 2> images;  ///< The textures of kPlanetImages, asked for at startup.
    float countdown = 0.0F;                             ///< Seconds before the next planet.
};
RTYPE_ECS_RESOURCE(BackgroundPlanets, "game.BackgroundPlanets");

}  // namespace rtype_duel
