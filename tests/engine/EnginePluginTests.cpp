/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EnginePluginTests
*/

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <utility>

#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/engine/EnginePlugin.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/engine/ProjectSettings.hpp"

namespace {
using rtype::ecs::App;
using rtype::ecs::RunMode;
using rtype::engine::EnginePlugin;

/// @brief Writes a temporary project script, removed on destruction.
class TempProject {
  public:
    explicit TempProject(const std::string& content)
        : _path{std::filesystem::temp_directory_path() /
                std::format("rtype_engine_plugin_project_{}.luau", ++_counter)} {
        std::ofstream{_path, std::ios::binary} << content;
    }

    ~TempProject() { std::filesystem::remove(_path); }
    TempProject(const TempProject&) = delete;
    TempProject& operator=(const TempProject&) = delete;
    TempProject(TempProject&&) = delete;
    TempProject& operator=(TempProject&&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return _path; }

  private:
    static inline int _counter = 0;  ///< Makes the file names unique.
    std::filesystem::path _path;     ///< Where the script is written.
};

/// @brief Expects adding the engine plugin on @p project to throw an engine RuntimeError mentioning @p text.
void expectEngineError(RunMode mode, const TempProject& project, std::string_view text) {
    try {
        App app{mode};
        app.addPlugin(EnginePlugin{project.path()});
        FAIL() << "no exception";
    } catch (const rtype::engine::exceptions::RuntimeError& error) {
        EXPECT_NE(std::string_view{error.what()}.find(text), std::string_view::npos) << error.what();
    }
}
}  // namespace

// Adding the engine plugin only registers the others: nothing is opened until run(), so these tests need no window.

TEST(EnginePlugin, GivesAClientTheDefaultPlugins) {
    const TempProject project{""};
    App app{RunMode::kClient};
    app.addPlugin(EnginePlugin{project.path()});
    EXPECT_TRUE(app.hasPlugin(rtype::ecs::TimePlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::audio::AudioPlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::input::InputPlugin::kName));  // brought by the backend
    EXPECT_EQ(app.getSchedules().getRunMode(), RunMode::kClient);
}

TEST(EnginePlugin, GivesStandaloneTheDefaultPlugins) {
    const TempProject project{""};
    App app{RunMode::kStandalone};
    app.addPlugin(EnginePlugin{project.path()});
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
}

TEST(EnginePlugin, GivesAServerNoWindowAndNoSound) {
    const TempProject project{""};
    App app{RunMode::kServer};
    app.addPlugin(EnginePlugin{project.path()});
    EXPECT_TRUE(app.hasPlugin(rtype::ecs::TimePlugin::kName));
    EXPECT_FALSE(app.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
    EXPECT_FALSE(app.hasPlugin(rtype::plugins::audio::AudioPlugin::kName));
}

TEST(EnginePlugin, AddsThePluginsTheProjectListsForItsMode) {
    const TempProject project{R"(
engine.settings.plugins.client = { "physics", "text" }
engine.settings.plugins.server = { "physics" }
)"};
    App client{RunMode::kClient};
    client.addPlugin(EnginePlugin{project.path()});
    EXPECT_TRUE(client.hasPlugin(rtype::plugins::physics::PhysicsPlugin::kName));
    EXPECT_TRUE(client.hasPlugin(rtype::plugins::text::TextPlugin::kName));
    EXPECT_FALSE(client.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));  // the list replaces them

    App server{RunMode::kServer};
    server.addPlugin(EnginePlugin{project.path()});
    EXPECT_TRUE(server.hasPlugin(rtype::plugins::physics::PhysicsPlugin::kName));
    EXPECT_FALSE(server.hasPlugin(rtype::plugins::text::TextPlugin::kName));
}

TEST(EnginePlugin, KeepsTheDefaultsWhenOnlyAnotherModeIsListed) {
    const TempProject project{R"(engine.settings.plugins.server = { "physics" })"};
    App client{RunMode::kClient};
    client.addPlugin(EnginePlugin{project.path()});
    EXPECT_TRUE(client.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
    EXPECT_FALSE(client.hasPlugin(rtype::plugins::physics::PhysicsPlugin::kName));
}

TEST(EnginePlugin, ListsTheBackendAndAudioByName) {
    const TempProject project{R"(engine.settings.plugins.client = { "backend", "audio", "backend" })"};
    App app{RunMode::kClient};
    app.addPlugin(EnginePlugin{project.path()});  // "backend" twice: the second is skipped by name
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::audio::AudioPlugin::kName));
}

TEST(EnginePlugin, ConfiguresPhysicsFromTheSettings) {
    const TempProject project{R"(
engine.settings.plugins.server = { "physics" }
engine.settings.physics.gravity = { 0, 9.5 }
engine.settings.physics.unitsPerMeter = 16
)"};
    App app{RunMode::kServer};
    app.addPlugin(EnginePlugin{project.path()});
    const auto& world = app.getWorld().getResources().require<rtype::plugins::physics::PhysicsWorld>();
    EXPECT_FLOAT_EQ(world.getGravity().x, 0.0F);
    EXPECT_FLOAT_EQ(world.getGravity().y, 9.5F);
    EXPECT_FLOAT_EQ(world.getUnitsPerMeter(), 16.0F);
}

TEST(EnginePlugin, RejectsAGravityWithoutTwoNumbers) {
    const TempProject project{R"(
engine.settings.plugins.server = { "physics" }
engine.settings.physics.gravity = { 1, 2, 3 }
)"};
    expectEngineError(RunMode::kServer, project, "'physics.gravity' needs two numbers");
}

TEST(EnginePlugin, RejectsAnUnknownPluginNamingIt) {
    const TempProject project{R"(engine.settings.plugins.client = { "backend", "phisycs" })"};
    expectEngineError(RunMode::kClient, project, "'phisycs'");
}

TEST(EnginePlugin, LetsTheProjectSeeItsModeAndBuild) {
    const TempProject project{R"(
engine.settings.mode = if engine.isServer and engine.isClient then "standalone"
    elseif engine.isServer then "server" else "client"
engine.settings.debug = engine.isDebug
)"};
    for (const auto& [mode, name] : {std::pair{RunMode::kClient, "client"}, std::pair{RunMode::kServer, "server"},
                                     std::pair{RunMode::kStandalone, "standalone"}}) {
        App app{mode};
        app.addPlugin(EnginePlugin{project.path()});
        const auto& settings = app.getWorld().getResources().require<rtype::engine::ProjectSettings>().values;
        EXPECT_EQ(settings.getString("mode", ""), name);
#ifdef NDEBUG
        EXPECT_FALSE(settings.getBool("debug", true));
#else
        EXPECT_TRUE(settings.getBool("debug", false));
#endif
    }
}

TEST(EnginePlugin, IsNamedRtypeEngineAndKeepsItsProject) {
    const EnginePlugin plugin{"game/project.luau"};
    EXPECT_EQ(plugin.getName(), "rtype.engine");
    EXPECT_EQ(plugin.getProject(), std::filesystem::path{"game/project.luau"});
}

TEST(EnginePlugin, InsertsTheProjectSettingsAsAResource) {
    const TempProject project{R"(engine.settings.game.lives = 3)"};
    App app{RunMode::kServer};
    app.addPlugin(EnginePlugin{project.path()});
    const auto& settings = app.getWorld().getResources().require<rtype::engine::ProjectSettings>();
    EXPECT_EQ(settings.values.getNumber("game.lives", 0), 3.0);
}

TEST(EnginePlugin, IsBuiltOnceWhenAddedTwice) {
    const TempProject project{R"(engine.settings.plugins.server = { "physics" })"};
    App app{RunMode::kServer};
    app.addPlugin(EnginePlugin{project.path()}).addPlugin(EnginePlugin{"does/not/exist.luau"});  // skipped by name
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::physics::PhysicsPlugin::kName));
}
