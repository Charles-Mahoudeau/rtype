/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpawnTilemap
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::tilemap {

/// @brief Where spawnTilemap() puts a map.
struct TilemapPlacement {
    glm::vec2 position{0.0F};         ///< The world point of the map's origin (its top-left corner).
    glm::vec2 scale{1.0F};            ///< World units per map pixel.
    std::int32_t firstDrawLayer = 0;  ///< The draw layer of the map's bottom layer; each layer above adds one.
};

/// @brief Spawns one entity per layer of the map @p map, bottom first, to draw it.
///
/// @details Each entity gets a Transform (the map's position plus the layer's offset, scaled), a TilemapLayer (the
/// layer, and its draw layer) and a Parallax: the layer's parallax factor and scroll speed, its origin where the camera
/// sees the map's parallax origin. Hidden layers are spawned too (the plugin skips them), so layer indices and entities
/// match.
/// @return The entities, bottom layer first; none if @p map is unknown or failed to load.
std::vector<ecs::Entity> spawnTilemap(ecs::Commands& commands, const TilemapAssets& maps, asset::AssetId map,
                                      const TilemapPlacement& placement = {});

}  // namespace rtype::plugins::tilemap
