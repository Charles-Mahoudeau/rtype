/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapAssetsTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::plugins::asset::AssetId;
using rtype::plugins::tilemap::Tilemap;
using rtype::plugins::tilemap::TilemapAssets;

namespace {
/// @brief A text file in the temporary directory, removed when the test ends.
class TemporaryFile {
  public:
    TemporaryFile(const std::string& name, std::string_view text)
        : _path{std::filesystem::temp_directory_path() / name} {
        std::ofstream file{_path};
        file << text;
    }
    ~TemporaryFile() { std::filesystem::remove(_path); }
    TemporaryFile(const TemporaryFile& other) = delete;
    TemporaryFile& operator=(const TemporaryFile& other) = delete;
    TemporaryFile(TemporaryFile&& other) = delete;
    TemporaryFile& operator=(TemporaryFile&& other) = delete;

    [[nodiscard]] const std::filesystem::path& getPath() const noexcept { return _path; }

  private:
    std::filesystem::path _path;  ///< Where the file was written.
};

/// @brief A map whose external tileset and image layer use the same image.
constexpr std::string_view kMap = R"({
  "tilewidth": 16, "tileheight": 16,
  "tilesets": [ { "firstgid": 1, "source": "rtype-tilemap-tests.tsj" } ],
  "layers": [
    { "type": "imagelayer", "name": "sky", "image": "rtype-tilemap-tests.png", "repeatx": true },
    { "type": "tilelayer", "name": "ground", "width": 2, "height": 1, "data": [1, 2] }
  ]
})";

/// @brief The external tileset of kMap.
constexpr std::string_view kTileset = R"({ "name": "tiles", "image": "rtype-tilemap-tests.png", "imagewidth": 32,
  "imageheight": 16, "tilewidth": 16, "tileheight": 16, "columns": 2, "tilecount": 2 })";
}  // namespace

static_assert(rtype::ecs::Resource<TilemapAssets>);

TEST(TilemapAssets, ReadsAMapAndItsExternalTilesets) {
    const TemporaryFile tileset{"rtype-tilemap-tests.tsj", kTileset};
    const TemporaryFile map{"rtype-tilemap-tests.tmj", kMap};
    TilemapAssets maps;
    const AssetId id = maps.load(map.getPath());
    EXPECT_EQ(id, AssetId::fromPath(map.getPath()));
    const Tilemap* loaded = maps.get(id);
    ASSERT_NE(loaded, nullptr);
    EXPECT_EQ(loaded->getLayers().size(), 2U);
    EXPECT_EQ(loaded->getTilesets().size(), 1U);
}

TEST(TilemapAssets, AsksForEachImageOnceRepeatedIfALayerRepeatsIt) {
    const TemporaryFile tileset{"rtype-tilemap-tests.tsj", kTileset};
    const TemporaryFile map{"rtype-tilemap-tests.tmj", kMap};
    TilemapAssets maps;
    static_cast<void>(maps.load(map.getPath()));
    static_cast<void>(maps.load(map.getPath()));  // Already known: nothing new.
    const std::vector<TilemapAssets::TextureRequest> requests = maps.takeTextureRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(std::filesystem::path{requests.at(0).path}.filename(), "rtype-tilemap-tests.png");
    EXPECT_TRUE(requests.at(0).desc.repeat);
    EXPECT_TRUE(maps.takeTextureRequests().empty());
}

TEST(TilemapAssets, RecordsAMapThatCannotBeRead) {
    TilemapAssets maps;
    const AssetId id = maps.load(std::filesystem::temp_directory_path() / "rtype-no-such-map.tmj");
    EXPECT_EQ(maps.get(id), nullptr);
    ASSERT_NE(maps.getEntry(id), nullptr);
    EXPECT_FALSE(maps.getEntry(id)->error.empty());
}

TEST(TilemapAssets, RecordsAMapWhoseTilesetIsMissing) {
    const TemporaryFile map{"rtype-tilemap-tests.tmj", kMap};
    TilemapAssets maps;
    const AssetId id = maps.load(map.getPath());
    EXPECT_EQ(maps.get(id), nullptr);
    EXPECT_NE(maps.getEntry(id)->error.find("rtype-tilemap-tests.tsj"), std::string::npos);
}

TEST(TilemapAssets, AddsAndUnloadsAMapBuiltInCode) {
    TilemapAssets maps;
    const AssetId id = maps.add("level", Tilemap{{8, 8}, {}, {}});
    EXPECT_NE(maps.get(id), nullptr);
    EXPECT_EQ(maps.getCount(), 1U);
    EXPECT_TRUE(maps.unload(id));
    EXPECT_EQ(maps.get(id), nullptr);
    EXPECT_FALSE(maps.unload(id));
}
