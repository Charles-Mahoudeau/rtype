/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TiledJsonTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/backend/graphics/Color.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/detail/TiledJson.hpp"
#include "plugins/tilemap/exceptions/TilemapExceptions.hpp"

using rtype::plugins::tilemap::LayerKind;
using rtype::plugins::tilemap::MapLayer;
using rtype::plugins::tilemap::Tilemap;
using rtype::plugins::tilemap::exceptions::TilemapException;

namespace {
/// @brief A map with an embedded and an external tileset, an image layer, a group holding a tile layer, and an object
/// layer (skipped).
constexpr std::string_view kLevel = R"({
  "orientation": "orthogonal", "tilewidth": 16, "tileheight": 16, "width": 3, "height": 2,
  "parallaxoriginx": 10, "parallaxoriginy": 20,
  "tilesets": [
    { "firstgid": 1, "name": "walls", "image": "walls.png", "imagewidth": 64, "imageheight": 32,
      "tilewidth": 16, "tileheight": 16, "columns": 4, "tilecount": 8, "tileoffset": { "x": 1, "y": 2 } },
    { "firstgid": 9, "source": "props.tsj" }
  ],
  "layers": [
    { "type": "imagelayer", "name": "sky", "image": "../backgrounds/sky.png", "repeatx": true, "parallaxx": 0.5,
      "opacity": 0.5, "tintcolor": "#ff0000" },
    { "type": "group", "name": "level", "offsetx": 10, "offsety": 5, "parallaxx": 0.5, "opacity": 0.5,
      "properties": [ { "name": "scrollx", "type": "float", "value": -3 } ],
      "layers": [
        { "type": "tilelayer", "name": "walls", "width": 3, "height": 2, "offsetx": 1, "parallaxx": 0.5,
          "opacity": 0.5, "data": [1, 0, 2, 0, 2147483651, 0] }
      ] },
    { "type": "objectgroup", "name": "spawns", "objects": [] }
  ]
})";

/// @brief The external tileset props.tsj.
constexpr std::string_view kProps = R"({ "name": "props", "image": "props.png", "imagewidth": 32, "imageheight": 32,
  "tilewidth": 32, "tileheight": 32, "columns": 1, "tilecount": 1 })";

/// @return The map of @p text, read from assets/levels/level.tmj, its external tilesets from @p tilesets.
Tilemap parse(std::string_view text, std::vector<std::filesystem::path>* asked = nullptr) {
    return rtype::plugins::tilemap::detail::parseTiledMap(text, "assets/levels/level.tmj",
                                                          [asked](const std::filesystem::path& path) {
                                                              if (asked != nullptr) {
                                                                  asked->push_back(path);
                                                              }
                                                              return std::string{kProps};
                                                          });
}

/// @return A map whose layers are @p layers, a JSON array.
std::string mapOf(std::string_view layers, std::string_view tilesets = "[]") {
    return std::format(R"({{ "tilewidth": 8, "tileheight": 8, "tilesets": {}, "layers": {} }})", tilesets, layers);
}
}  // namespace

TEST(TiledJson, ReadsTheGridAndTheParallaxOrigin) {
    const Tilemap map = parse(kLevel);
    EXPECT_EQ(map.getTileSize(), (glm::uvec2{16, 16}));
    EXPECT_EQ(map.getParallaxOrigin(), (glm::vec2{10.0F, 20.0F}));
}

TEST(TiledJson, ReadsEmbeddedAndExternalTilesets) {
    std::vector<std::filesystem::path> asked;
    const Tilemap map = parse(kLevel, &asked);
    ASSERT_EQ(map.getTilesets().size(), 2U);
    EXPECT_EQ(map.getTilesets().at(0).imagePath, "assets/levels/walls.png");
    EXPECT_EQ(map.getTilesets().at(0).tileOffset, (glm::vec2{1.0F, 2.0F}));
    EXPECT_EQ(map.getTilesets().at(1).firstGid, 9U);
    EXPECT_EQ(map.getTilesets().at(1).imagePath, "assets/levels/props.png");
    EXPECT_EQ(map.getTilesets().at(1).tileSize, (glm::uvec2{32, 32}));
    ASSERT_EQ(asked.size(), 1U);
    EXPECT_EQ(asked.at(0).generic_string(), "assets/levels/props.tsj");
}

TEST(TiledJson, ReadsAnImageLayer) {
    const Tilemap map = parse(kLevel);
    const MapLayer& sky = map.getLayers().at(0);
    EXPECT_EQ(sky.kind, LayerKind::kImage);
    EXPECT_EQ(sky.name, "sky");
    EXPECT_EQ(sky.imagePath, "assets/backgrounds/sky.png");
    EXPECT_TRUE(sky.repeatX);
    EXPECT_FALSE(sky.repeatY);
    EXPECT_EQ(sky.parallax, (glm::vec2{0.5F, 1.0F}));
    EXPECT_EQ(sky.tint, (rtype::plugins::backend::graphics::Color{1.0F, 0.0F, 0.0F, 1.0F}));
}

TEST(TiledJson, FlattensGroupsIntoTheirLayers) {
    const Tilemap map = parse(kLevel);
    ASSERT_EQ(map.getLayers().size(), 2U);  // The object layer is skipped.
    const MapLayer& walls = map.getLayers().at(1);
    EXPECT_EQ(walls.name, "level/walls");
    EXPECT_EQ(walls.offset, (glm::vec2{11.0F, 5.0F}));    // Added.
    EXPECT_EQ(walls.parallax, (glm::vec2{0.25F, 1.0F}));  // Multiplied.
    EXPECT_FLOAT_EQ(walls.opacity, 0.25F);
    EXPECT_EQ(walls.scroll, (glm::vec2{-3.0F, 0.0F}));
}

TEST(TiledJson, ReadsTheTilesWithTheirFlags) {
    const Tilemap map = parse(kLevel);
    const MapLayer& walls = map.getLayers().at(1);
    EXPECT_EQ(walls.kind, LayerKind::kTiles);
    EXPECT_EQ(walls.getTile({0, 0}), 1U);
    EXPECT_EQ(walls.getTile({1, 0}), 0U);
    EXPECT_EQ(walls.getTile({2, 0}), 2U);
    EXPECT_EQ(walls.getTile({1, 1}), 3U | rtype::plugins::tilemap::kFlipHorizontal);
}

TEST(TiledJson, ReadsTheChunksOfAnInfiniteMap) {
    std::string data = "7";
    for (int cell = 1; cell < 256; ++cell) {
        data += ",0";
    }
    const Tilemap map = parse(mapOf(std::format(
        R"([{{ "type": "tilelayer", "name": "ground", "chunks": [ {{ "x": -16, "y": 16, "width": 16, "height": 16,
        "data": [{}] }} ] }}])",
        data)));
    EXPECT_EQ(map.getLayers().at(0).getTile({-16, 16}), 7U);
}

TEST(TiledJson, RejectsTextThatIsNotJson) { EXPECT_THROW(static_cast<void>(parse("{ not json")), TilemapException); }

TEST(TiledJson, RejectsAnotherOrientation) {
    EXPECT_THROW(static_cast<void>(parse(R"({ "orientation": "isometric", "layers": [] })")), TilemapException);
}

TEST(TiledJson, RejectsBase64TileData) {
    EXPECT_THROW(
        static_cast<void>(parse(
            mapOf(R"([{ "type": "tilelayer", "name": "b", "width": 1, "encoding": "base64", "data": "AAAAAA==" }])"))),
        TilemapException);
}

TEST(TiledJson, RejectsATilesetOfSeparateImages) {
    EXPECT_THROW(static_cast<void>(parse(mapOf("[]", R"([{ "firstgid": 1, "name": "props", "tilecount": 1,
        "tilewidth": 8, "tileheight": 8, "columns": 0, "tiles": [] }])"))),
                 TilemapException);
}

TEST(TiledJson, RejectsATintThatIsNotAColour) {
    EXPECT_THROW(static_cast<void>(parse(mapOf(
                     R"([{ "type": "imagelayer", "name": "sky", "image": "sky.png", "tintcolor": "#zz0000" }])"))),
                 TilemapException);
}
