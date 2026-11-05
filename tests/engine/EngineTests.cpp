/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EngineTests
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
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/Exceptions.hpp"

namespace {
using rtype::ecs::RunMode;
using rtype::engine::Engine;

/// @brief Writes a temporary project script, removed on destruction.
class TempProject {
  public:
    explicit TempProject(const std::string& content)
        : _path{std::filesystem::temp_directory_path() / std::format("rtype_engine_project_{}.luau", ++_counter)} {
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

/// @brief Expects building an engine on @p project to throw an engine RuntimeError mentioning @p text.
void expectEngineError(RunMode mode, const TempProject& project, std::string_view text) {
    try {
        const Engine engine{mode, project.path()};
        FAIL() << "no exception";
    } catch (const rtype::engine::exceptions::RuntimeError& error) {
        EXPECT_NE(std::string_view{error.what()}.find(text), std::string_view::npos) << error.what();
    }
}
}  // namespace

// Building an engine only registers its plugins: nothing is opened until run(), so these tests need no window.

TEST(Engine, GivesAClientTheDefaultPlugins) {
    const TempProject project{""};
    const Engine engine{RunMode::kClient, project.path()};
    const rtype::ecs::App& app = engine.getApp();
    EXPECT_TRUE(app.hasPlugin(rtype::ecs::TimePlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::audio::AudioPlugin::kName));
    EXPECT_TRUE(app.hasPlugin(rtype::plugins::input::InputPlugin::kName));  // brought by the backend
    EXPECT_EQ(app.getSchedules().getRunMode(), RunMode::kClient);
}

TEST(Engine, GivesStandaloneTheDefaultPlugins) {
    const TempProject project{""};
    const Engine engine{RunMode::kStandalone, project.path()};
    EXPECT_TRUE(engine.getApp().hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
}

TEST(Engine, GivesAServerNoWindowAndNoSound) {
    const TempProject project{""};
    const Engine engine{RunMode::kServer, project.path()};
    const rtype::ecs::App& app = engine.getApp();
    EXPECT_TRUE(app.hasPlugin(rtype::ecs::TimePlugin::kName));
    EXPECT_FALSE(app.hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
    EXPECT_FALSE(app.hasPlugin(rtype::plugins::audio::AudioPlugin::kName));
}

TEST(Engine, AddsThePluginsTheProjectListsForItsMode) {
    const TempProject project{R"(
engine.settings.plugins.client = { "physics", "text" }
engine.settings.plugins.server = { "physics" }
)"};
    const Engine client{RunMode::kClient, project.path()};
    EXPECT_TRUE(client.getApp().hasPlugin(rtype::plugins::physics::PhysicsPlugin::kName));
    EXPECT_TRUE(client.getApp().hasPlugin(rtype::plugins::text::TextPlugin::kName));
    EXPECT_FALSE(client.getApp().hasPlugin(rtype::plugins::backend::BackendPlugin::kName));  // the list replaces them

    const Engine server{RunMode::kServer, project.path()};
    EXPECT_TRUE(server.getApp().hasPlugin(rtype::plugins::physics::PhysicsPlugin::kName));
    EXPECT_FALSE(server.getApp().hasPlugin(rtype::plugins::text::TextPlugin::kName));
}

TEST(Engine, KeepsTheDefaultsWhenOnlyAnotherModeIsListed) {
    const TempProject project{R"(engine.settings.plugins.server = { "physics" })"};
    const Engine client{RunMode::kClient, project.path()};
    EXPECT_TRUE(client.getApp().hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
    EXPECT_FALSE(client.getApp().hasPlugin(rtype::plugins::physics::PhysicsPlugin::kName));
}

TEST(Engine, ListsTheBackendAndAudioByName) {
    const TempProject project{R"(engine.settings.plugins.client = { "backend", "audio", "backend" })"};
    const Engine engine{RunMode::kClient, project.path()};  // "backend" twice: the second is skipped by name
    EXPECT_TRUE(engine.getApp().hasPlugin(rtype::plugins::backend::BackendPlugin::kName));
    EXPECT_TRUE(engine.getApp().hasPlugin(rtype::plugins::audio::AudioPlugin::kName));
}

TEST(Engine, ConfiguresPhysicsFromTheSettings) {
    const TempProject project{R"(
engine.settings.plugins.server = { "physics" }
engine.settings.physics.gravity = { 0, 9.5 }
engine.settings.physics.unitsPerMeter = 16
)"};
    const Engine engine{RunMode::kServer, project.path()};
    const auto& world = engine.getApp().getWorld().getResources().require<rtype::plugins::physics::PhysicsWorld>();
    EXPECT_FLOAT_EQ(world.getGravity().x, 0.0F);
    EXPECT_FLOAT_EQ(world.getGravity().y, 9.5F);
    EXPECT_FLOAT_EQ(world.getUnitsPerMeter(), 16.0F);
}

TEST(Engine, RejectsAGravityWithoutTwoNumbers) {
    const TempProject project{R"(
engine.settings.plugins.server = { "physics" }
engine.settings.physics.gravity = { 1, 2, 3 }
)"};
    expectEngineError(RunMode::kServer, project, "'physics.gravity' needs two numbers");
}

TEST(Engine, RejectsAnUnknownPluginNamingIt) {
    const TempProject project{R"(engine.settings.plugins.client = { "backend", "phisycs" })"};
    expectEngineError(RunMode::kClient, project, "'phisycs'");
}

TEST(Engine, LetsTheProjectSeeItsModeAndBuild) {
    const TempProject project{R"(
engine.settings.mode = if engine.isServer and engine.isClient then "standalone"
    elseif engine.isServer then "server" else "client"
engine.settings.debug = engine.isDebug
)"};
    for (const auto& [mode, name] : {std::pair{RunMode::kClient, "client"}, std::pair{RunMode::kServer, "server"},
                                     std::pair{RunMode::kStandalone, "standalone"}}) {
        const Engine engine{mode, project.path()};
        EXPECT_EQ(engine.getSettings().getString("mode", ""), name);
#ifdef NDEBUG
        EXPECT_FALSE(engine.getSettings().getBool("debug", true));
#else
        EXPECT_TRUE(engine.getSettings().getBool("debug", false));
#endif
    }
}
