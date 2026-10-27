/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AtlasAssetsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <glm/ext/vector_uint2.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "TestImages.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/exceptions/AssetExceptions.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::plugins::asset::AssetId;
using rtype::plugins::sprite::AtlasAssets;
using rtype::plugins::sprite::SpriteAtlas;

namespace {
/// @brief A file in the temporary directory, removed when the test ends.
class TemporaryFile {
  public:
    TemporaryFile(const std::string& name, std::span<const std::byte> content)
        : _path{std::filesystem::temp_directory_path() / name} {
        std::ofstream file{_path, std::ios::binary};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        file.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
    }
    TemporaryFile(const std::string& name, std::string_view text)
        : TemporaryFile{name, std::as_bytes(std::span{text})} {}
    ~TemporaryFile() { std::filesystem::remove(_path); }
    TemporaryFile(const TemporaryFile& other) = delete;
    TemporaryFile& operator=(const TemporaryFile& other) = delete;
    TemporaryFile(TemporaryFile&& other) = delete;
    TemporaryFile& operator=(TemporaryFile&& other) = delete;

    [[nodiscard]] const std::filesystem::path& getPath() const noexcept { return _path; }

  private:
    std::filesystem::path _path;  ///< Where the file was written.
};

/// @brief An Aseprite atlas of two frames and one animation, its image next to it.
constexpr std::string_view kAtlasJson = R"({
  "frames": {
    "a": { "frame": { "x": 0, "y": 0, "w": 1, "h": 1 } },
    "b": { "frame": { "x": 1, "y": 0, "w": 1, "h": 1 } }
  },
  "meta": { "image": "rtype-atlas-tests.png", "frameTags": [ { "name": "blink", "from": 0, "to": 1 } ] }
})";
}  // namespace

static_assert(rtype::ecs::Resource<AtlasAssets>);

TEST(AtlasAssets, ReadsAnAtlasFileAtOnce) {
    const TemporaryFile json{"rtype-atlas-tests.json", kAtlasJson};
    AtlasAssets atlases;
    const AssetId id = atlases.load(json.getPath());
    EXPECT_EQ(id, AssetId::fromPath(json.getPath()));
    const SpriteAtlas* atlas = atlases.get(id);
    ASSERT_NE(atlas, nullptr);
    EXPECT_EQ(atlas->getFrames().size(), 2U);
    EXPECT_NE(atlas->findAnimation(rtype::plugins::sprite::animationId("blink")), nullptr);
}

TEST(AtlasAssets, AsksForTheTextureOfEachAtlasOnce) {
    const TemporaryFile json{"rtype-atlas-tests.json", kAtlasJson};
    AtlasAssets atlases;
    static_cast<void>(atlases.load(json.getPath()));
    static_cast<void>(atlases.load(json.getPath()));  // Already known: nothing new.
    const std::vector<AtlasAssets::TextureRequest> requests = atlases.takeTextureRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(std::filesystem::path{requests.at(0).path}.filename(), "rtype-atlas-tests.png");
    EXPECT_TRUE(requests.at(0).pixels.empty());
    EXPECT_TRUE(atlases.takeTextureRequests().empty());
}

TEST(AtlasAssets, RecordsAnAtlasThatCannotBeRead) {
    AtlasAssets atlases;
    const AssetId id = atlases.load(std::filesystem::temp_directory_path() / "rtype-no-such-atlas.json");
    EXPECT_EQ(atlases.get(id), nullptr);
    const AtlasAssets::Entry* entry = atlases.getEntry(id);
    ASSERT_NE(entry, nullptr);
    EXPECT_FALSE(entry->error.empty());
    EXPECT_TRUE(atlases.takeTextureRequests().empty());
}

TEST(AtlasAssets, AddsAnAtlasBuiltInCode) {
    AtlasAssets atlases;
    const AssetId id = atlases.add("enemy", SpriteAtlas::grid("assets/enemy.png", {8.0F, 8.0F}, 4));
    ASSERT_NE(atlases.get(id), nullptr);
    EXPECT_EQ(atlases.get(id)->getFrames().size(), 4U);
    EXPECT_EQ(atlases.getCount(), 1U);
    const std::vector<AtlasAssets::TextureRequest> requests = atlases.takeTextureRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(requests.at(0).path, "assets/enemy.png");
}

TEST(AtlasAssets, PacksImagesIntoOneTexture) {
    const TemporaryFile left{"rtype-pack-left.png", rtype::test::plugins::twoPixelPng()};
    const TemporaryFile right{"rtype-pack-right.png", rtype::test::plugins::twoPixelPng()};
    AtlasAssets atlases;
    const std::array images{left.getPath(), right.getPath()};
    const AssetId id = atlases.pack("fx", images);
    const SpriteAtlas* atlas = atlases.get(id);
    ASSERT_NE(atlas, nullptr);
    ASSERT_EQ(atlas->getFrames().size(), 2U);
    EXPECT_TRUE(atlas->findFrame("rtype-pack-left"));
    EXPECT_TRUE(atlas->findFrame("rtype-pack-right"));
    EXPECT_FLOAT_EQ(atlas->getFrames().at(0).source.getWidth(), 2.0F);

    const std::vector<AtlasAssets::TextureRequest> requests = atlases.takeTextureRequests();
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(requests.at(0).path, "fx");
    const glm::uvec2 size = requests.at(0).desc.size;
    EXPECT_EQ(requests.at(0).pixels.size(), static_cast<std::size_t>(size.x) * size.y * 4);
}

TEST(AtlasAssets, RecordsAPackOfAMissingImage) {
    AtlasAssets atlases;
    const std::array images{std::filesystem::temp_directory_path() / "rtype-no-such-image.png"};
    const AssetId id = atlases.pack("broken", images);
    EXPECT_EQ(atlases.get(id), nullptr);
    EXPECT_FALSE(atlases.getEntry(id)->error.empty());
}

TEST(AtlasAssets, UnloadsAnAtlas) {
    AtlasAssets atlases;
    const AssetId id = atlases.add("enemy", SpriteAtlas{"assets/enemy.png"});
    EXPECT_TRUE(atlases.unload(id));
    EXPECT_EQ(atlases.get(id), nullptr);
    EXPECT_FALSE(atlases.unload(id));
}
