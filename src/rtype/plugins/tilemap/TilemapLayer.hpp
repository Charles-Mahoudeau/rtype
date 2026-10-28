/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapLayer
*/

#pragma once

#include <cstdint>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::tilemap {

/// @brief Draws one layer of a map (the component "rtype.TilemapLayer"): where comes from the entity's GlobalTransform
/// (the map's origin for this layer, and its scale; a layer is not rotated).
///
/// @details spawnTilemap() spawns one entity per layer of a map. The TilemapPlugin draws a tile layer's chunks that the
/// camera shows, and an image layer once (repeated to fill the view if Tiled says so), through the SpriteQueue, sorted
/// with the sprites by `drawLayer`. Every field is reflected.
struct TilemapLayer {
    core::asset::AssetId map;    ///< The map, from TilemapAssets::load().
    std::uint32_t layer = 0;     ///< Index in Tilemap::getLayers().
    std::int32_t drawLayer = 0;  ///< Draw order among sprites: as SpriteRenderer::layer, higher on top.
};
RTYPE_ECS_COMPONENT(TilemapLayer, "rtype.TilemapLayer", map, layer, drawLayer);

}  // namespace rtype::plugins::tilemap
