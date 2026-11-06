/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TileDrawing
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <optional>

#include "plugins/graphics/VisibleArea.hpp"
#include "plugins/tilemap/Export.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"

namespace rtype::plugins::tilemap::detail {

/// @brief Where a layer is drawn: its entity's GlobalTransform (a layer is not rotated).
struct LayerPlacement {
    glm::vec2 position{0.0F};  ///< The world point of the map's origin for this layer.
    glm::vec2 scale{1.0F};     ///< World units per map pixel.
};

/// @brief A box in the world.
struct Bounds {
    glm::vec2 min{0.0F};  ///< Top-left corner.
    glm::vec2 max{0.0F};  ///< Bottom-right corner.
};

/// @return The colour a layer's sprites are drawn with: its tint, its alpha multiplied by its opacity.
[[nodiscard]] RTYPE_PLUGIN_TILEMAP_API contract::renderer::Color layerColor(const MapLayer& layer);

/// @return The sprite of the tile id @p gid in @p cell, as Tiled draws it: aligned on the cell's bottom-left corner,
/// moved by its tileset's offset, flipped by its flags (diagonal first, then horizontal and vertical).
/// @param texture The tileset's texture, loaded.
[[nodiscard]] RTYPE_PLUGIN_TILEMAP_API contract::renderer::Sprite tileSprite(const Tilemap& map, const Tileset& tileset,
                                                                             contract::renderer::TextureId texture,
                                                                             std::uint32_t gid, glm::ivec2 cell,
                                                                             const LayerPlacement& placement,
                                                                             const contract::renderer::Color& color);

/// @return The box the tiles of the chunk at @p origin may cover in the world, with the map's overhang.
[[nodiscard]] RTYPE_PLUGIN_TILEMAP_API Bounds chunkBounds(const Tilemap& map, glm::ivec2 origin,
                                                          const LayerPlacement& placement);

/// @return The sprite of the image layer @p layer: its image at its place or, repeated, enough copies of it (one
/// sprite, the texture sampled with repeat) to cover @p visible along the repeated axes; nothing if it is not visible.
/// @param texture Its image, loaded with TextureDesc::repeat when it repeats.
/// @param imageSize The image's size, in pixels.
[[nodiscard]] RTYPE_PLUGIN_TILEMAP_API std::optional<contract::renderer::Sprite> imageSprite(
    const MapLayer& layer, contract::renderer::TextureId texture, glm::uvec2 imageSize, const LayerPlacement& placement,
    const graphics::VisibleArea& visible);

}  // namespace rtype::plugins::tilemap::detail
