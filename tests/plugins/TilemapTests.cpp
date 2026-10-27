/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <optional>
#include <vector>

#include "plugins/tilemap/Tilemap.hpp"

using rtype::plugins::tilemap::MapLayer;
using rtype::plugins::tilemap::Tilemap;
using rtype::plugins::tilemap::Tileset;

namespace {
/// @return A tileset of @p count 16x16 tiles from @p firstGid.
Tileset tilesetOf(std::uint32_t firstGid, std::uint32_t count) {
    return Tileset{.tileSize = {16, 16}, .firstGid = firstGid, .tileCount = count, .columns = 4};
}
}  // namespace

TEST(Tilemap, StoresTilesByChunks) {
    MapLayer layer;
    const std::array<std::uint32_t, 4> gids{1, 2, 3, 4};
    rtype::plugins::tilemap::setTiles(layer, {14, 0}, 4, gids);
    ASSERT_EQ(layer.chunks.size(), 2U);  // Cells 14 and 15, then 16 and 17.
    EXPECT_EQ(layer.getTile({15, 0}), 2U);
    EXPECT_EQ(layer.getTile({16, 0}), 3U);
    EXPECT_EQ(layer.getTile({20, 0}), 0U);
}

TEST(Tilemap, StoresTilesAtNegativeCells) {
    MapLayer layer;
    const std::array<std::uint32_t, 1> gids{5};
    rtype::plugins::tilemap::setTiles(layer, {-1, -1}, 1, gids);
    ASSERT_EQ(layer.chunks.size(), 1U);
    EXPECT_EQ(layer.chunks.at(0).origin, (glm::ivec2{-16, -16}));
    EXPECT_EQ(layer.getTile({-1, -1}), 5U);
}

TEST(Tilemap, DropsAChunkOnceItsTilesAreCleared) {
    MapLayer layer;
    const std::array<std::uint32_t, 1> tile{5};
    const std::array<std::uint32_t, 1> empty{0};
    rtype::plugins::tilemap::setTiles(layer, {3, 3}, 1, tile);
    rtype::plugins::tilemap::setTiles(layer, {3, 3}, 1, empty);
    EXPECT_TRUE(layer.chunks.empty());
}

TEST(Tilemap, FindsATileInItsImageWithMarginAndSpacing) {
    const Tileset tileset{.tileSize = {16, 16}, .columns = 3, .spacing = 2, .margin = 1};
    const rtype::plugins::backend::math::Rect source = tileset.getSource(4);  // Second column, second row.
    EXPECT_FLOAT_EQ(source.getX(), 19.0F);
    EXPECT_FLOAT_EQ(source.getY(), 19.0F);
    EXPECT_FLOAT_EQ(source.getWidth(), 16.0F);
}

TEST(Tilemap, FindsTheTilesetOfATileIgnoringItsFlags) {
    const Tilemap map{{16, 16}, {tilesetOf(5, 4), tilesetOf(1, 4)}, {}};
    ASSERT_NE(map.findTileset(6U | rtype::plugins::tilemap::kFlipHorizontal), nullptr);
    EXPECT_EQ(map.findTileset(6)->firstGid, 5U);
    EXPECT_EQ(map.findTileset(4)->firstGid, 1U);
    EXPECT_EQ(map.findTileset(9), nullptr);
    EXPECT_EQ(map.findTileset(0), nullptr);
}

TEST(Tilemap, MeasuresHowFarTilesReachPastTheirCell) {
    Tileset wide = tilesetOf(1, 1);
    wide.tileSize = {32, 16};
    wide.tileOffset = {0.0F, -4.0F};
    EXPECT_FLOAT_EQ((Tilemap{{16, 16}, {wide}, {}}.getOverhang()), 16.0F);
    EXPECT_FLOAT_EQ((Tilemap{{16, 16}, {tilesetOf(1, 1)}, {}}.getOverhang()), 0.0F);
}

TEST(Tilemap, FindsALayerByName) {
    const Tilemap map{{16, 16}, {}, {MapLayer{.name = "sky"}, MapLayer{.name = "ground"}}, {8.0F, 4.0F}};
    EXPECT_EQ(map.findLayer("ground"), std::optional<std::uint32_t>{1});
    EXPECT_FALSE(map.findLayer("water"));
    EXPECT_EQ(map.getParallaxOrigin(), (glm::vec2{8.0F, 4.0F}));
}
