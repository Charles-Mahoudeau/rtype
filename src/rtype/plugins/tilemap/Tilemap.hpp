/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Tilemap
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/math/Rect.hpp"

namespace rtype::plugins::tilemap {

/// @brief Tiled's flag on a tile id: the tile is flipped horizontally.
inline constexpr std::uint32_t kFlipHorizontal = 0x80000000U;
/// @brief Tiled's flag on a tile id: the tile is flipped vertically.
inline constexpr std::uint32_t kFlipVertical = 0x40000000U;
/// @brief Tiled's flag on a tile id: the tile is flipped across its top-left to bottom-right diagonal (applied first).
inline constexpr std::uint32_t kFlipDiagonal = 0x20000000U;
/// @brief The bits of a tile id that name the tile, without Tiled's flip flags (and its hexagonal rotation one).
inline constexpr std::uint32_t kGidMask = 0x0FFFFFFFU;
/// @brief Tiles per side of a chunk: a tile layer is stored, and culled, by squares of this many tiles.
inline constexpr std::int32_t kChunkSize = 16;

/// @brief An image cut into same-size tiles, as a Tiled tileset: tile `firstGid + n` of a map is its tile n.
struct Tileset {
    std::string name;              ///< As named in Tiled.
    std::string imagePath;         ///< The image, as TextureAssets loads it (relative to the working directory).
    core::asset::AssetId texture;  ///< AssetId::fromPath(imagePath).
    glm::uvec2 imageSize{0};       ///< In pixels.
    glm::uvec2 tileSize{0};        ///< In pixels.
    std::uint32_t firstGid = 1;    ///< The map's id of its first tile.
    std::uint32_t tileCount = 0;   ///< How many tiles it has.
    std::uint32_t columns = 0;     ///< Tiles per row of the image.
    std::uint32_t spacing = 0;     ///< Pixels between two tiles.
    std::uint32_t margin = 0;      ///< Pixels around the tiles, at the image's edges.
    glm::vec2 tileOffset{0.0F};    ///< Added to where each of its tiles is drawn, in pixels.

    /// @return Where its tile @p localId (0 for the first) is in the image, in pixels.
    [[nodiscard]] backend::math::Rect getSource(std::uint32_t localId) const;
};

/// @brief A square of kChunkSize x kChunkSize cells of a tile layer.
struct TileChunk {
    glm::ivec2 origin{0};  ///< Its top-left cell, a multiple of kChunkSize.
    std::array<std::uint32_t, static_cast<std::size_t>(kChunkSize) * kChunkSize> gids{};  ///< Row by row; 0: empty.
    std::uint32_t count = 0;                                                              ///< Its non-empty cells.
};

/// @brief What a layer holds.
enum class LayerKind : std::uint8_t {
    kTiles,  ///< Tiles on the map's grid (Tiled's tile layer).
    kImage,  ///< One image, possibly repeated (Tiled's image layer).
};

/// @brief One layer of a map, as Tiled draws it: groups are flattened into their layers (offsets added, parallax
/// factors, opacities and tints multiplied).
struct MapLayer {
    std::string name;                     ///< Its name in Tiled, prefixed by its groups' ("group/layer").
    LayerKind kind = LayerKind::kTiles;   ///< What it holds.
    glm::vec2 offset{0.0F};               ///< Where it is drawn from the map's origin, in pixels.
    glm::vec2 parallax{1.0F};             ///< Parallax factor (Parallax::factor): 1 moves with the map.
    glm::vec2 scroll{0.0F};               ///< Pixels per second it scrolls on its own (custom properties scrollx and
                                          ///< scrolly, an extension of this plugin).
    float opacity = 1.0F;                 ///< Multiplies its alpha.
    backend::graphics::Color tint{1.0F};  ///< Multiplies its colours.
    bool visible = true;                  ///< Hidden layers are kept but not drawn.
    std::vector<TileChunk> chunks;        ///< kTiles: the chunks that have tiles.
    std::string imagePath;                ///< kImage: the image, as TextureAssets loads it.
    core::asset::AssetId image;           ///< kImage: AssetId::fromPath(imagePath).
    bool repeatX = false;                 ///< kImage: repeated to fill the view horizontally.
    bool repeatY = false;                 ///< kImage: repeated to fill the view vertically.

    /// @return The tile id (with its flip flags) in @p cell, 0 if empty.
    [[nodiscard]] std::uint32_t getTile(glm::ivec2 cell) const;
};

/// @brief Writes the tile ids @p gids, @p width per row, into @p layer from @p origin (in cells); 0 clears a cell.
void setTiles(MapLayer& layer, glm::ivec2 origin, std::uint32_t width, std::span<const std::uint32_t> gids);

/// @brief A level made of tile and image layers on a grid, as Tiled saves an orthogonal map: what TilemapAssets loads
/// and a TilemapLayer entity draws.
class Tilemap {
  public:
    /// @param tileSize The grid's cell, in pixels.
    /// @param tilesets Ordered by firstGid.
    /// @param parallaxOrigin The point of the map, in pixels, where the camera sees every layer at its unscrolled
    /// place.
    Tilemap(glm::uvec2 tileSize, std::vector<Tileset> tilesets, std::vector<MapLayer> layers,
            glm::vec2 parallaxOrigin = glm::vec2{0.0F});
    Tilemap() = default;
    ~Tilemap() = default;
    Tilemap(const Tilemap& other) = default;
    Tilemap& operator=(const Tilemap& other) = default;
    Tilemap(Tilemap&& other) noexcept = default;
    Tilemap& operator=(Tilemap&& other) noexcept = default;

    /// @return The grid's cell, in pixels.
    [[nodiscard]] glm::uvec2 getTileSize() const noexcept { return _tileSize; }
    /// @return The tilesets, by firstGid.
    [[nodiscard]] const std::vector<Tileset>& getTilesets() const noexcept { return _tilesets; }
    /// @return The layers, bottom first.
    [[nodiscard]] const std::vector<MapLayer>& getLayers() const noexcept { return _layers; }
    /// @return Where the camera sees every layer unscrolled, in pixels from the map's origin.
    [[nodiscard]] glm::vec2 getParallaxOrigin() const noexcept { return _parallaxOrigin; }

    /// @return The index of the layer named @p name, or nothing.
    [[nodiscard]] std::optional<std::uint32_t> findLayer(std::string_view name) const;

    /// @return The tileset of the tile id @p gid (its flip flags are ignored), or nullptr if no tileset has it.
    [[nodiscard]] const Tileset* findTileset(std::uint32_t gid) const;

    /// @return How far a tile may reach past its cell, in pixels: tiles bigger than the grid and tile offsets.
    [[nodiscard]] float getOverhang() const noexcept;

  private:
    glm::uvec2 _tileSize{0};          ///< The grid's cell, in pixels.
    std::vector<Tileset> _tilesets;   ///< By firstGid.
    std::vector<MapLayer> _layers;    ///< Bottom first.
    glm::vec2 _parallaxOrigin{0.0F};  ///< Where the camera sees the layers unscrolled.
};

}  // namespace rtype::plugins::tilemap
