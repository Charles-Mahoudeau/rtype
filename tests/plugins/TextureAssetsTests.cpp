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
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "plugins/backend/TextureAssets.hpp"
#include "plugins/backend/exceptions/AssetExceptions.hpp"
#include "plugins/backend/graphics/Texture.hpp"

namespace {
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
    rtype::plugins::backend::TextureAssets assets;                    ///< The textures loaded.
};
}  // namespace

TEST(TextureAssets, DecodesAnImageAndCreatesTheTexture) {
    Fixture fixture;
    const rtype::plugins::backend::graphics::TextureId id = rtype::plugins::backend::loadTexture(
        fixture.assets, fixture.renderer, "two.png", rtype::test::plugins::twoPixelPng());
    ASSERT_EQ(fixture.state->createdTextures.size(), 1U);
    EXPECT_EQ(fixture.state->createdTextures[0].size, glm::uvec2(2, 1));
    std::vector<std::byte> expected = rgba(255, 0, 0, 255);
    const std::vector<std::byte> green = rgba(0, 255, 0, 128);
    expected.insert(expected.end(), green.begin(), green.end());
    EXPECT_EQ(fixture.state->createdPixels[0], expected);
    EXPECT_EQ(fixture.assets.find("two.png"), id);
    ASSERT_NE(fixture.assets.getInfo(id), nullptr);
    EXPECT_EQ(fixture.assets.getInfo(id)->size, glm::uvec2(2, 1));
}

TEST(TextureAssets, ForwardsTheOptions) {
    Fixture fixture;
    static_cast<void>(rtype::plugins::backend::loadTexture(
        fixture.assets, fixture.renderer, "smooth.png", rtype::test::plugins::twoPixelPng(),
        {.filter = rtype::plugins::backend::graphics::TextureFilter::kLinear, .repeat = true}));
    ASSERT_EQ(fixture.state->createdTextures.size(), 1U);
    EXPECT_EQ(fixture.state->createdTextures[0].filter, rtype::plugins::backend::graphics::TextureFilter::kLinear);
    EXPECT_TRUE(fixture.state->createdTextures[0].repeat);
}

TEST(TextureAssets, LoadsANameOnlyOnce) {
    Fixture fixture;
    const auto first = rtype::plugins::backend::loadTexture(fixture.assets, fixture.renderer, "two.png",
                                                            rtype::test::plugins::twoPixelPng());
    const auto second = rtype::plugins::backend::loadTexture(fixture.assets, fixture.renderer, "two.png",
                                                             rtype::test::plugins::twoPixelPng());
    EXPECT_EQ(first, second);
    EXPECT_EQ(fixture.state->createdTextures.size(), 1U);
    EXPECT_EQ(fixture.assets.getCount(), 1U);
}

TEST(TextureAssets, RefusesBytesThatAreNotAnImage) {
    Fixture fixture;
    const std::array<std::byte, 4> garbage{std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};
    EXPECT_THROW(
        static_cast<void>(rtype::plugins::backend::loadTexture(fixture.assets, fixture.renderer, "bad.png", garbage)),
        rtype::plugins::backend::exceptions::AssetLoadException);
    EXPECT_TRUE(fixture.state->createdTextures.empty());
    EXPECT_EQ(fixture.assets.getCount(), 0U);
}

TEST(TextureAssets, LoadsAFileByItsPath) {
    const TemporaryPng png{"rtype-texture-assets-test.png"};
    Fixture fixture;
    const auto id = rtype::plugins::backend::loadTexture(fixture.assets, fixture.renderer, png.getPath());
    EXPECT_EQ(fixture.assets.find(png.getPath().generic_string()), id);
    EXPECT_EQ(fixture.assets.getInfo(id)->size, glm::uvec2(2, 1));
    EXPECT_EQ(rtype::plugins::backend::loadTexture(fixture.assets, fixture.renderer, png.getPath()), id);
    EXPECT_EQ(fixture.state->createdTextures.size(), 1U);
}

TEST(TextureAssets, RefusesAMissingFile) {
    Fixture fixture;
    EXPECT_THROW(
        static_cast<void>(rtype::plugins::backend::loadTexture(
            fixture.assets, fixture.renderer, std::filesystem::temp_directory_path() / "rtype-no-such-texture.png")),
        rtype::plugins::backend::exceptions::AssetLoadException);
    EXPECT_EQ(fixture.assets.getCount(), 0U);
}
