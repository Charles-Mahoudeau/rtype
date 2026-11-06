/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderWatcherTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "core/config/Settings.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderConfig.hpp"
#include "plugins/shader/ShaderWatcher.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::core::asset::AssetId;
using rtype::plugins::shader::ShaderAssets;
using rtype::plugins::shader::ShaderConfig;
using rtype::plugins::shader::ShaderWatcher;

static_assert(rtype::ecs::Resource<ShaderWatcher>);

namespace {
constexpr std::string_view kPath = "assets/shaders/tint.glsl";  ///< The shader, in the game directory.
constexpr std::string_view kFirst = "uniform vec4 first;\nvoid fragment() { COLOR *= first; }\n";
constexpr std::string_view kSecond = "uniform vec4 second;\nvoid fragment() { COLOR *= second; }\n";
constexpr std::string_view kBroken = "void fragment() { COLOR = notDeclared; }\n";

/// @brief A game directory of its own, removed when the test ends.
class ShaderWatcherTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::filesystem::create_directories(_game / "assets/shaders");
        _shaders.setDirectory(_game);
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_game, error);
    }

    /// @brief Writes the file @p path of the game, dated a second after the last one so that it reads as changed.
    void write(const std::string_view path, const std::string_view text) {
        const std::filesystem::path file = _game / path;
        std::ofstream{file, std::ios::binary} << text;
        _time += std::chrono::seconds{1};
        std::filesystem::last_write_time(file, _time);
    }

    /// @return The name of the first `uniform` of the shader, the way these tests tell its versions apart.
    [[nodiscard]] std::string firstValue(const AssetId id) const {
        return _shaders.get(id)->materialValues.front().name;
    }

    std::filesystem::path _game{std::filesystem::temp_directory_path() /
                                std::format("rtype_shader_watcher_{:08x}", std::random_device{}())};
    std::filesystem::file_time_type _time{std::filesystem::file_time_type::clock::now()};  ///< Of the last write.
    ShaderAssets _shaders;                                                                 ///< The game's shaders.
};
}  // namespace

TEST_F(ShaderWatcherTest, ReloadsALoadedShaderWhenItsFileChanges) {
    write(kPath, kFirst);
    const AssetId id = _shaders.load(kPath);
    ShaderWatcher watcher{_game};
    watcher.poll(_shaders);
    EXPECT_TRUE(_shaders.takeReloaded().empty());

    write(kPath, kSecond);
    watcher.poll(_shaders);
    EXPECT_EQ(_shaders.takeReloaded(), (std::vector<AssetId>{id}));
    EXPECT_EQ(firstValue(id), "second");

    watcher.poll(_shaders);
    EXPECT_TRUE(_shaders.takeReloaded().empty());
}

TEST_F(ShaderWatcherTest, FollowsTheSubfolders) {
    std::filesystem::create_directories(_game / "assets/shaders/enemies");
    write("assets/shaders/enemies/tint.glsl", kFirst);
    const AssetId id = _shaders.load("assets/shaders/enemies/tint.glsl");
    ShaderWatcher watcher{_game};
    write("assets/shaders/enemies/tint.glsl", kSecond);
    watcher.poll(_shaders);
    EXPECT_EQ(firstValue(id), "second");
}

TEST_F(ShaderWatcherTest, IgnoresAFileTheGameDidNotLoad) {
    ShaderWatcher watcher{_game};
    write(kPath, kFirst);
    watcher.poll(_shaders);
    EXPECT_EQ(_shaders.getCount(), 0U);
}

TEST_F(ShaderWatcherTest, CompilesAShaderWhoseFileAppears) {
    const AssetId id = _shaders.load(kPath);
    ASSERT_EQ(_shaders.get(id), nullptr);
    ShaderWatcher watcher{_game};
    write(kPath, kFirst);
    watcher.poll(_shaders);
    ASSERT_NE(_shaders.get(id), nullptr);
    EXPECT_EQ(_shaders.takeAdded(), (std::vector<AssetId>{id}));
}

TEST_F(ShaderWatcherTest, KeepsTheLastVersionWhenAnEditDoesNotCompile) {
    write(kPath, kFirst);
    const AssetId id = _shaders.load(kPath);
    ShaderWatcher watcher{_game};
    write(kPath, kBroken);
    watcher.poll(_shaders);
    EXPECT_TRUE(_shaders.takeReloaded().empty());
    EXPECT_EQ(firstValue(id), "first");

    write(kPath, kSecond);
    watcher.poll(_shaders);
    EXPECT_EQ(firstValue(id), "second");
}

TEST_F(ShaderWatcherTest, ToleratesAMissingFolder) {
    std::filesystem::remove_all(_game / "assets");
    ShaderWatcher watcher{_game};
    watcher.poll(_shaders);
    EXPECT_EQ(_shaders.getCount(), 0U);
}

TEST_F(ShaderWatcherTest, UpdateWaitsForTheInterval) {
    write(kPath, kFirst);
    const AssetId id = _shaders.load(kPath);
    ShaderWatcher watcher{_game};
    write(kPath, kSecond);
    const ShaderWatcher::Clock::time_point start = ShaderWatcher::Clock::now();
    watcher.update(_shaders, start - ShaderWatcher::kInterval);
    EXPECT_EQ(firstValue(id), "first");
    watcher.update(_shaders, start + ShaderWatcher::kInterval * 2);
    EXPECT_EQ(firstValue(id), "second");
}

TEST(ShaderConfig, ReadsTheGameDirectoryAndHotReload) {
    rtype::core::config::Settings settings;
    settings.set(std::string{ShaderConfig::kGameDirectoryKey}, "games/duel");
    settings.set("shader.hotReload", true);
    const ShaderConfig config = ShaderConfig::fromSettings(settings);
    EXPECT_EQ(config.gameDirectory, std::filesystem::path{"games/duel"});
    EXPECT_TRUE(config.hotReload);

    settings.set("shader.hotReload", false);
    EXPECT_FALSE(ShaderConfig::fromSettings(settings).hotReload);
}

TEST(ShaderConfig, RejectsAnUnknownKey) {
    rtype::core::config::Settings settings;
    settings.set("shader.hotReloadd", true);
    EXPECT_ANY_THROW(static_cast<void>(ShaderConfig::fromSettings(settings)));
}
