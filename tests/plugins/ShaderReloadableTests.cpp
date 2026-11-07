/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderReloadableTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderReloadable.hpp"
#include "rtype/core/reload/ChangeSet.hpp"
#include "rtype/core/reload/HotReloader.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/core/reload/ReloadReport.hpp"

using rtype::core::asset::AssetId;
using rtype::core::reload::ChangeSet;
using rtype::core::reload::HotReloader;
using rtype::core::reload::ReloadOutcome;
using rtype::plugins::shader::ShaderAssets;
using rtype::plugins::shader::ShaderReloadable;

namespace {
constexpr std::string_view kPath = "shaders/tint.glsl";  ///< The shader, in the assets.
constexpr std::string_view kFirst = "uniform vec4 first;\nvoid fragment() { COLOR *= first; }\n";
constexpr std::string_view kSecond = "uniform vec4 second;\nvoid fragment() { COLOR *= second; }\n";
constexpr std::string_view kBroken = "void fragment() { COLOR = notDeclared; }\n";

/// @brief A game directory of its own, removed when the test ends.
class ShaderReloadableTest : public ::testing::Test {
  protected:
    void SetUp() override {
        std::filesystem::create_directories(_game / "assets/shaders");
        _shaders.setDirectory(_game / "assets");
    }
    void TearDown() override {
        std::error_code error;
        std::filesystem::remove_all(_game, error);
    }

    /// @brief Writes the file @p path of the assets, dated a second after the last one so that it reads as changed.
    void write(const std::string_view path, const std::string_view text) {
        const std::filesystem::path file = _game / "assets" / path;
        std::ofstream{file, std::ios::binary} << text;
        _time += std::chrono::seconds{1};
        std::filesystem::last_write_time(file, _time);
    }

    /// @return The name of the first `uniform` of the shader, the way these tests tell its versions apart.
    [[nodiscard]] std::string firstValue(const AssetId id) const {
        return _shaders.get(id)->materialValues.front().name;
    }

    /// @brief Runs the reloader twice, as the debounce wants the same stamp two times.
    std::vector<rtype::core::reload::ReloadReport> settle(HotReloader& reloader) {
        static_cast<void>(reloader.poll());
        return reloader.poll();
    }

    std::filesystem::path _game{std::filesystem::temp_directory_path() /
                                std::format("rtype_shader_reloadable_{:08x}", std::random_device{}())};
    std::filesystem::file_time_type _time{std::filesystem::file_time_type::clock::now()};  ///< Of the last write.
    ShaderAssets _shaders;                                                                 ///< The game's shaders.
    std::vector<std::string> _log;                                                         ///< What the reloader said.
};
}  // namespace

TEST_F(ShaderReloadableTest, ReloadsALoadedShaderWhenItsFileChanges) {
    write(kPath, kFirst);
    const AssetId id = _shaders.load(kPath);
    ShaderReloadable reloadable{&_shaders, "assets"};

    const ReloadOutcome outcome = reloadable.reload(ChangeSet{.modified = {"assets/shaders/tint.glsl"}});
    EXPECT_EQ(outcome.status, ReloadOutcome::Status::kApplied);
    EXPECT_EQ(outcome.what, (std::vector<std::string>{"shader shaders/tint.glsl"}));
    EXPECT_EQ(_shaders.takeReloaded(), (std::vector<AssetId>{id}));
    write(kPath, kSecond);
    EXPECT_EQ(reloadable.reload(ChangeSet{.modified = {"assets/shaders/tint.glsl"}}).status,
              ReloadOutcome::Status::kApplied);
    EXPECT_EQ(firstValue(id), "second");
}

TEST_F(ShaderReloadableTest, IgnoresAFileTheGameDidNotLoad) {
    write(kPath, kFirst);
    ShaderReloadable reloadable{&_shaders, "assets"};
    const ReloadOutcome outcome = reloadable.reload(ChangeSet{.added = {"assets/shaders/tint.glsl"}});
    EXPECT_EQ(outcome.status, ReloadOutcome::Status::kApplied);
    EXPECT_TRUE(outcome.what.empty());
    EXPECT_EQ(_shaders.getCount(), 0U);
}

TEST_F(ShaderReloadableTest, ReportsAFailureAndKeepsTheLastVersion) {
    write(kPath, kFirst);
    const AssetId id = _shaders.load(kPath);
    ShaderReloadable reloadable{&_shaders, "assets"};
    write(kPath, kBroken);

    const ReloadOutcome outcome = reloadable.reload(ChangeSet{.modified = {"assets/shaders/tint.glsl"}});
    EXPECT_EQ(outcome.status, ReloadOutcome::Status::kFailed);
    EXPECT_NE(outcome.message.find("shaders/tint.glsl"), std::string::npos);
    EXPECT_EQ(firstValue(id), "first");
}

TEST_F(ShaderReloadableTest, FollowsTheFilesThroughTheReloader) {
    write(kPath, kFirst);
    const AssetId id = _shaders.load(kPath);
    HotReloader reloader{_game, [this](const std::string_view line) { _log.emplace_back(line); }};
    reloader.add(std::make_unique<ShaderReloadable>(&_shaders, "assets"));
    ASSERT_EQ(reloader.size(), 1U);

    write(kPath, kSecond);
    const auto reports = settle(reloader);
    ASSERT_EQ(reports.size(), 1U);
    EXPECT_EQ(reports.front().source, "shaders");
    EXPECT_EQ(firstValue(id), "second");
    ASSERT_EQ(_log.size(), 1U);
    EXPECT_NE(_log.front().find("assets/shaders/tint.glsl"), std::string::npos);
}

TEST_F(ShaderReloadableTest, ToleratesAMissingFolder) {
    std::filesystem::remove_all(_game / "assets/shaders");
    HotReloader reloader{_game, [](std::string_view) {}};
    reloader.add(std::make_unique<ShaderReloadable>(&_shaders, "assets"));
    EXPECT_TRUE(settle(reloader).empty());
}
