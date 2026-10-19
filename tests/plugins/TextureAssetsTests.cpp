/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TextureAssetsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "plugins/backend/exceptions/AssetExceptions.hpp"
#include "plugins/backend/graphics/AssetId.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
constexpr rtype::plugins::backend::graphics::TextureId kShip{.index = 0, .generation = 1};

/// @brief A file holding the test PNG, removed when the test ends.
class TemporaryPng {
  public:
    explicit TemporaryPng(const char* name) : _path{std::filesystem::temp_directory_path() / name} {
        std::ofstream file{_path, std::ios::binary};
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        file.write(reinterpret_cast<const char*>(rtype::test::plugins::kTwoPixelPng),
                   static_cast<std::streamsize>(sizeof(rtype::test::plugins::kTwoPixelPng)));
    }
    ~TemporaryPng() { std::filesystem::remove(_path); }
    TemporaryPng(const TemporaryPng& other) = delete;
    TemporaryPng& operator=(const TemporaryPng& other) = delete;
    TemporaryPng(TemporaryPng&& other) = delete;
    TemporaryPng& operator=(TemporaryPng&& other) = delete;

    [[nodiscard]] const std::filesystem::path& getPath() const noexcept { return _path; }

  private:
    std::filesystem::path _path;  ///< Where the PNG was written.
};

/// @return The bytes of a pixel: red, green, blue, alpha.
std::vector<std::byte> rgba(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha) {
    return {std::byte{red}, std::byte{green}, std::byte{blue}, std::byte{alpha}};
}

/// @brief A fake renderer with its recorded state.
struct Fixture {
    std::shared_ptr<rtype::test::plugins::FakeBackendState> state{
        std::make_shared<rtype::test::plugins::FakeBackendState>()};  ///< What the renderer recorded.
    rtype::test::plugins::FakeRenderer renderer{state};               ///< Creates the textures.
    rtype::plugins::backend::TextureAssets assets;                    ///< The textures asked for.

    /// @brief Runs what the BackendPlugin runs each frame.
    void update() { rtype::plugins::backend::updateTextures(assets, renderer); }
};
}  // namespace

TEST(TextureAssets, DecodesAnImageAndCreatesTheTexture) {
    Fixture fixture;
    const auto id = fixture.assets.load("two.png", rtype::test::plugins::twoPixelPng());
    EXPECT_TRUE(fixture.state->createdTextures.empty());

    fixture.update();

    ASSERT_EQ(fixture.state->createdTextures.size(), 1U);
    EXPECT_EQ(fixture.state->createdTextures[0].size, glm::uvec2(2, 1));
    std::vector<std::byte> expected = rgba(255, 0, 0, 255);
    const std::vector<std::byte> green = rgba(0, 255, 0, 128);
    expected.insert(expected.end(), green.begin(), green.end());
    EXPECT_EQ(fixture.state->createdPixels[0], expected);
    EXPECT_EQ(fixture.assets.getTexture(id),
              (rtype::plugins::backend::graphics::TextureId{.index = 0, .generation = 1}));
    EXPECT_EQ(fixture.assets.get(id)->desc.size, glm::uvec2(2, 1));
    EXPECT_TRUE(fixture.assets.getQueued().empty());
}

TEST(TextureAssets, ForwardsTheDesc) {
    Fixture fixture;
    static_cast<void>(
        fixture.assets.load("smooth.png", rtype::test::plugins::twoPixelPng(),
                            {.filter = rtype::plugins::backend::graphics::TextureFilter::kLinear, .repeat = true}));
    fixture.update();
    ASSERT_EQ(fixture.state->createdTextures.size(), 1U);
    EXPECT_EQ(fixture.state->createdTextures[0].filter, rtype::plugins::backend::graphics::TextureFilter::kLinear);
    EXPECT_TRUE(fixture.state->createdTextures[0].repeat);
}

TEST(TextureAssets, CreatesATextureOnlyOnce) {
    Fixture fixture;
    static_cast<void>(fixture.assets.load("two.png", rtype::test::plugins::twoPixelPng()));
    fixture.update();
    static_cast<void>(fixture.assets.load("two.png", rtype::test::plugins::twoPixelPng()));
    fixture.update();
    EXPECT_EQ(fixture.state->createdTextures.size(), 1U);
}

TEST(TextureAssets, MarksBytesThatAreNotAnImageAsFailed) {
    Fixture fixture;
    const std::array<std::byte, 4> garbage{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    const auto id = fixture.assets.load("bad.png", garbage);
    fixture.update();
    EXPECT_TRUE(fixture.state->createdTextures.empty());
    EXPECT_EQ(fixture.assets.get(id)->state, rtype::plugins::backend::AssetState::kFailed);
    EXPECT_NE(fixture.assets.get(id)->error.find("cannot decode"), std::string::npos);
}

TEST(TextureAssets, LoadsAFileByItsPath) {
    const TemporaryPng png{"rtype-texture-assets-test.png"};
    Fixture fixture;
    const auto id = fixture.assets.load(png.getPath());
    fixture.update();
    EXPECT_EQ(id, rtype::plugins::backend::AssetId::fromPath(png.getPath()));
    EXPECT_TRUE(fixture.assets.getTexture(id));
    EXPECT_EQ(fixture.assets.get(id)->desc.size, glm::uvec2(2, 1));
}

TEST(TextureAssets, MarksAMissingFileAsFailedAndLoadsTheOthers) {
    Fixture fixture;
    const auto missing = fixture.assets.load(std::filesystem::temp_directory_path() / "rtype-no-such-texture.png");
    const auto present = fixture.assets.load("two.png", rtype::test::plugins::twoPixelPng());
    fixture.update();
    EXPECT_EQ(fixture.assets.get(missing)->state, rtype::plugins::backend::AssetState::kFailed);
    EXPECT_TRUE(fixture.assets.getTexture(present));
}

TEST(TextureAssets, DestroysTheUnloadedTextures) {
    Fixture fixture;
    const auto id = fixture.assets.load("two.png", rtype::test::plugins::twoPixelPng());
    fixture.update();
    const auto texture = *fixture.assets.getTexture(id);
    fixture.assets.unload(id);
    EXPECT_TRUE(fixture.state->destroyedTextures.empty());

    fixture.update();

    EXPECT_EQ(fixture.state->destroyedTextures, std::vector{texture});
}

static_assert(rtype::ecs::Resource<rtype::plugins::backend::TextureAssets>);

TEST(TextureAssets, StartsEmpty) {
    const rtype::plugins::backend::TextureAssets assets;
    EXPECT_EQ(assets.getCount(), 0U);
    EXPECT_TRUE(assets.getQueued().empty());
    EXPECT_EQ(assets.get(rtype::plugins::backend::AssetId::fromPath("ship.png")), nullptr);
}

TEST(TextureAssets, LoadQueuesTheAssetAndReturnsTheIdOfItsPath) {
    rtype::plugins::backend::TextureAssets assets;
    const rtype::plugins::backend::AssetId id =
        assets.load("assets/ship.png", {.filter = rtype::plugins::backend::graphics::TextureFilter::kLinear});
    EXPECT_EQ(id, rtype::plugins::backend::AssetId::fromPath("assets/ship.png"));
    EXPECT_EQ(assets.getQueued(), std::vector{id});
    const auto* entry = assets.get(id);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->path, "assets/ship.png");
    EXPECT_EQ(entry->state, rtype::plugins::backend::AssetState::kQueued);
    EXPECT_EQ(entry->desc.filter, rtype::plugins::backend::graphics::TextureFilter::kLinear);
    EXPECT_FALSE(assets.getTexture(id));
}

TEST(TextureAssets, AsksForAPathOnlyOnce) {
    rtype::plugins::backend::TextureAssets assets;
    const auto first = assets.load("assets/ship.png");
    const auto second =
        assets.load("assets/./ship.png", {.filter = rtype::plugins::backend::graphics::TextureFilter::kLinear});
    EXPECT_EQ(first, second);
    EXPECT_EQ(assets.getCount(), 1U);
    EXPECT_EQ(assets.getQueued().size(), 1U);
    EXPECT_EQ(assets.get(first)->desc.filter, rtype::plugins::backend::graphics::TextureFilter::kNearest);
}

TEST(TextureAssets, KeepsTheBytesOfAnAssetGivenInMemory) {
    rtype::plugins::backend::TextureAssets assets;
    const std::array<std::byte, 2> bytes{std::byte{1}, std::byte{2}};
    const auto id = assets.load("embedded/logo.png", bytes);
    EXPECT_EQ(id, rtype::plugins::backend::AssetId::fromPath("embedded/logo.png"));
    EXPECT_EQ(assets.get(id)->encoded, (std::vector<std::byte>{bytes.begin(), bytes.end()}));
}

TEST(TextureAssets, SetLoadedRecordsTheHandleAndTheDesc) {
    rtype::plugins::backend::TextureAssets assets;
    const std::array<std::byte, 1> bytes{std::byte{1}};
    const auto id = assets.load("ship.png", bytes);
    assets.setLoaded(id, kShip, {.size = {32, 16}});
    EXPECT_TRUE(assets.getQueued().empty());
    EXPECT_EQ(assets.getTexture(id), kShip);
    EXPECT_EQ(assets.get(id)->state, rtype::plugins::backend::AssetState::kLoaded);
    EXPECT_EQ(assets.get(id)->desc.size, glm::uvec2(32, 16));
    EXPECT_TRUE(assets.get(id)->encoded.empty());
}

TEST(TextureAssets, SetFailedRecordsWhy) {
    rtype::plugins::backend::TextureAssets assets;
    const auto id = assets.load("missing.png");
    assets.setFailed(id, "cannot open the file");
    EXPECT_TRUE(assets.getQueued().empty());
    EXPECT_FALSE(assets.getTexture(id));
    EXPECT_EQ(assets.get(id)->state, rtype::plugins::backend::AssetState::kFailed);
    EXPECT_EQ(assets.get(id)->error, "cannot open the file");
}

TEST(TextureAssets, IgnoresResultsForAnAssetThatIsNotQueued) {
    rtype::plugins::backend::TextureAssets assets;
    const auto id = assets.load("ship.png");
    assets.setLoaded(id, kShip, {.size = {32, 16}});
    assets.setFailed(id, "late");
    assets.setLoaded(rtype::plugins::backend::AssetId::fromPath("other.png"), kShip, {});
    EXPECT_EQ(assets.get(id)->state, rtype::plugins::backend::AssetState::kLoaded);
    EXPECT_EQ(assets.getCount(), 1U);
}

TEST(TextureAssets, UnloadGivesTheHandleBackForTheBackend) {
    rtype::plugins::backend::TextureAssets assets;
    const auto id = assets.load("ship.png");
    assets.setLoaded(id, kShip, {.size = {32, 16}});
    EXPECT_TRUE(assets.unload(id));
    EXPECT_EQ(assets.get(id), nullptr);
    EXPECT_EQ(assets.takeUnloaded(), std::vector{kShip});
    EXPECT_TRUE(assets.takeUnloaded().empty());
    EXPECT_FALSE(assets.unload(id));
}

TEST(TextureAssets, UnloadingAQueuedAssetCancelsIt) {
    rtype::plugins::backend::TextureAssets assets;
    const auto id = assets.load("ship.png");
    EXPECT_TRUE(assets.unload(id));
    EXPECT_TRUE(assets.getQueued().empty());
    EXPECT_TRUE(assets.takeUnloaded().empty());
}

TEST(TextureAssets, LivesInTheWorldAsAResource) {
    rtype::ecs::World world;
    rtype::plugins::backend::TextureAssets assets;
    const auto id = assets.load("ship.png");
    world.getResources().insert(std::move(assets));
    EXPECT_NE(world.getResources().require<rtype::plugins::backend::TextureAssets>().get(id), nullptr);
    EXPECT_TRUE(world.getResources().getRegistry().findByName("rtype.TextureAssets"));
}
