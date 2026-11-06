/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapCalls
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <string_view>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::tilemap {

inline constexpr std::string_view kLoadTilemapCall = "rtype.tilemap.load";    ///< TilemapAssets::load().
inline constexpr std::string_view kSpawnTilemapCall = "rtype.tilemap.spawn";  ///< spawnTilemap().

/// @brief The arguments of `rtype.tilemap.load`: reads a Tiled `.tmj` file.
struct LoadTilemapArguments {
    ecs::String path;  ///< The map file.
};
RTYPE_ECS_CALL_DATA(LoadTilemapArguments, "rtype.tilemap.LoadTilemapArguments", path);

/// @brief The result of `rtype.tilemap.load`.
struct TilemapResult {
    core::asset::AssetId map;  ///< The map's id, for `rtype.tilemap.spawn`.
};
RTYPE_ECS_CALL_DATA(TilemapResult, "rtype.tilemap.TilemapResult", map);

/// @brief The arguments of `rtype.tilemap.spawn`: spawns one entity per layer of a loaded map (TilemapPlacement).
struct SpawnTilemapArguments {
    core::asset::AssetId map;        ///< The map, from `rtype.tilemap.load` (or its path).
    glm::vec2 position{0.0F};        ///< The world point of the map's top-left corner.
    glm::vec2 scale{1.0F};           ///< World units per map pixel.
    std::int32_t firstDrawLayer{0};  ///< The draw layer of the map's bottom layer; each layer above adds one.
};
RTYPE_ECS_CALL_DATA(SpawnTilemapArguments, "rtype.tilemap.SpawnTilemapArguments", map, position, scale, firstDrawLayer);

/// @brief The result of `rtype.tilemap.spawn`.
struct SpawnTilemapResult {
    std::uint32_t layers{0};  ///< How many layer entities were spawned; 0 if the map is unknown or failed to load.
};
RTYPE_ECS_CALL_DATA(SpawnTilemapResult, "rtype.tilemap.SpawnTilemapResult", layers);

/// @brief Registers the tilemap engine calls on @p app, for scripts: `rtype.tilemap.load` and `rtype.tilemap.spawn`,
/// on the `rtype.TilemapAssets` resource. Called by the TilemapPlugin.
void addTilemapCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin on @p registry: its library does it on every side, so a server gets
/// the ids of what it does not load, and so does building the plugin, for a program without libraries.
void addTilemapForms(ecs::PluginRegistry& registry);

}  // namespace rtype::plugins::tilemap
