/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GamePluginsTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "rtype/core/config/Settings.hpp"
#include "rtype/core/config/exceptions/ConfigExceptions.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/EngineConfig.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/GamePlugins.hpp"
#include "rtype/plugins/graphics/BackendRegistry.hpp"
#include "rtype/plugins/graphics/GraphicsPlugin.hpp"
#include "rtype/plugins/input/InputPlugin.hpp"
#include "rtype/plugins/physics/PhysicsPlugin.hpp"
#include "rtype/plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "rtype/plugins/transform/TransformPlugin.hpp"

namespace {
using rtype::core::config::Settings;
using rtype::ecs::RunMode;
using rtype::engine::GamePlugins;

/// @return Settings listing @p plugins.
Settings listing(std::vector<std::string> plugins) {
    Settings settings;
    settings.set(std::string{GamePlugins::kSettingsKey}, std::move(plugins));
    return settings;
}
}  // namespace

TEST(GamePlugins, AddsNothingWhenTheSettingsListNone) {
    const GamePlugins plugins = GamePlugins::fromSettings(Settings{}, RunMode::kStandalone);
    EXPECT_FALSE(plugins.transform);
    EXPECT_FALSE(plugins.physics);
    EXPECT_FALSE(plugins.physicsDebug);
}

TEST(GamePlugins, ReadsTheListedPluginsAndTheirSections) {
    Settings settings = listing({"transform", "physics", "physicsDebug"});
    settings.set("physics.gravity", std::vector<double>{0.0, 500.0});
    settings.set("physicsDebug.drawAll", true);

    const GamePlugins plugins = GamePlugins::fromSettings(settings, RunMode::kClient);

    EXPECT_TRUE(plugins.transform);
    ASSERT_TRUE(plugins.physics);
    EXPECT_FLOAT_EQ(plugins.physics->gravity.y, 500.0F);
    ASSERT_TRUE(plugins.physicsDebug);
    EXPECT_TRUE(plugins.physicsDebug->drawAll);
}

TEST(GamePlugins, SkipsThePhysicsDebugViewOnAServer) {
    const GamePlugins plugins = GamePlugins::fromSettings(listing({"physics", "physicsDebug"}), RunMode::kServer);
    EXPECT_TRUE(plugins.physics);
    EXPECT_FALSE(plugins.physicsDebug);
}

TEST(GamePlugins, RejectsAnUnknownPlugin) {
    EXPECT_THROW(static_cast<void>(GamePlugins::fromSettings(listing({"physic"}), RunMode::kServer)),
                 rtype::engine::exceptions::RuntimeError);
}

TEST(GamePlugins, ChecksTheSectionsOfPluginsItDoesNotAdd) {
    Settings settings;
    settings.set("physicsDebug.drawEverything", true);
    EXPECT_THROW(static_cast<void>(GamePlugins::fromSettings(settings, RunMode::kServer)),
                 rtype::core::config::exceptions::SettingsException);
}

TEST(GamePlugins, AddsItsPluginsWithTheirSettings) {
    Settings settings = listing({"physics", "physicsDebug"});
    settings.set("physicsDebug.enabled", false);
    rtype::ecs::App app;
    app.addPlugin(rtype::plugins::transform::TransformPlugin{});  // what the Engine and the program add first
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    app.addPlugin(rtype::plugins::graphics::GraphicsPlugin{
        std::make_unique<rtype::plugins::graphics::BackendRegistry>(), rtype::core::config::Settings{}});

    GamePlugins::fromSettings(settings, RunMode::kStandalone).addTo(app);

    EXPECT_TRUE(app.hasPlugin(rtype::plugins::physics::PhysicsPlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::physics::debug::PhysicsDebugPlugin::kName));
    EXPECT_FALSE(app.getWorld().getResources().require<rtype::plugins::physics::debug::PhysicsDebugSettings>().enabled);
}

TEST(GamePlugins, TheEngineAddsThemBeforeTheSetup) {
    const std::filesystem::path game = std::filesystem::temp_directory_path() / "rtype-engine-game-plugins";
    std::filesystem::remove_all(game);
    std::filesystem::create_directories(game);
    std::ofstream{game / rtype::engine::Engine::kProjectFile} << R"(
        engine.settings.network.port = 0
        engine.settings.plugins = { "physics" }
        engine.settings.physics.gravity = { 0, 10 }
    )";
    bool physicsBefore = false;
    rtype::engine::EngineConfig config{.mode = RunMode::kServer, .game = game};
    config.setup = [&physicsBefore](rtype::engine::Engine& engine) {
        physicsBefore = engine.app().hasPlugin(rtype::plugins::physics::PhysicsPlugin::kName);
    };

    {
        const rtype::engine::Engine engine{std::move(config)};
        EXPECT_TRUE(physicsBefore);  // so the game's setup and scripts can use the physics
    }
    std::error_code error;
    std::filesystem::remove_all(game, error);
}
