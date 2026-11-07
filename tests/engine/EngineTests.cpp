/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EngineTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "core/system/ExecutablePath.hpp"
#include "plugins/AssetDirectory.hpp"
#include "rtype/core/config/Settings.hpp"
#include "rtype/core/config/exceptions/ConfigExceptions.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/EngineConfig.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/replication/ReplicationPlugin.hpp"
#include "rtype/replication/server/ServerPlugin.hpp"

namespace rtype::test::engine {
/// @brief The build of the App a system runs in: 1 for the first one, then one more per reload.
struct Build {
    std::int32_t number;
};
RTYPE_ECS_RESOURCE(Build, "test.engine.Build", number);
}  // namespace rtype::test::engine

namespace {
using rtype::engine::Engine;
using rtype::engine::EngineConfig;
using rtype::engine::exceptions::RuntimeError;
using rtype::test::engine::Build;

/// @brief The project settings of the test games: a server on a port the system picks, so tests never collide.
constexpr std::string_view kProject = R"(engine.settings.network.port = engine.args.port or 0)";

/// @brief A game directory in the temporary directory, removed with it.
class GameDirectory {
  public:
    explicit GameDirectory(const std::string_view name, const std::string_view project = kProject)
        : _path{std::filesystem::temp_directory_path() / std::string{name}} {
        std::filesystem::remove_all(_path);
        std::filesystem::create_directories(_path);
        write(std::string{Engine::kProjectFile}, project);
    }

    ~GameDirectory() {
        std::error_code error;
        std::filesystem::remove_all(_path, error);
    }

    GameDirectory(const GameDirectory& other) = delete;
    GameDirectory& operator=(const GameDirectory& other) = delete;
    GameDirectory(GameDirectory&& other) = delete;
    GameDirectory& operator=(GameDirectory&& other) = delete;

    [[nodiscard]] const std::filesystem::path& path() const noexcept { return _path; }

    /// @brief Writes a file under the directory, creating its folders.
    void write(const std::string& relative, const std::string_view content) const {
        const auto file = _path / relative;
        std::filesystem::create_directories(file.parent_path());
        std::ofstream{file} << content;
    }

  private:
    std::filesystem::path _path;  ///< The directory.
};

/// @return A headless server config that runs @p game.
EngineConfig serverConfig(const GameDirectory& game, std::vector<std::string> args = {}) {
    return {.mode = rtype::ecs::RunMode::kServer, .game = game.path(), .args = std::move(args)};
}

/// @brief Update: stops the App once it runs in its second build.
void exitOnSecondBuild(rtype::ecs::Res<Build> build, rtype::ecs::EventWriter<rtype::ecs::AppExit> exit) {
    if (build->number == 2) {
        exit.send(rtype::ecs::AppExit{.code = build->number});
    }
}

/// @brief Update: stops the App at once.
void exitAtOnce(rtype::ecs::Res<Build> build, rtype::ecs::EventWriter<rtype::ecs::AppExit> exit) {
    exit.send(rtype::ecs::AppExit{.code = build->number});
}

/// @return A setup that numbers the builds in @p builds and adds @p system.
template <typename System>
auto countingSetup(std::int32_t& builds, System system) {
    return [&builds, system](Engine& engine) {
        engine.app().insertResource(Build{.number = ++builds});
        engine.app().addSystem(rtype::ecs::Phase::kUpdate, "test.exit", system).setSide(rtype::ecs::Side::kBoth);
    };
}
}  // namespace

TEST(Engine, FailsWithoutAProjectFile) {
    const GameDirectory game{"rtype-engine-no-project"};
    std::filesystem::remove(game.path() / Engine::kProjectFile);

    EXPECT_THROW(Engine{serverConfig(game)}, RuntimeError);
}

TEST(Engine, FindsTheGameGivenByTheGameFlag) {
    const GameDirectory game{"rtype-engine-game-flag"};
    const EngineConfig config{.mode = rtype::ecs::RunMode::kServer,
                              .game = game.path() / "missing",
                              .args = {"--game", game.path().string()}};

    const Engine engine{config};

    EXPECT_EQ(engine.gameDirectory(), game.path().lexically_normal());
    EXPECT_EQ(engine.commandLine().get("game"), game.path().string());
}

TEST(Engine, MakesTheGameDirectoryAbsolute) {
    const GameDirectory game{"rtype-engine-absolute"};
    // Work from the game's parent so the path is relative on every platform (on Windows the temporary directory can
    // be on another drive than the working directory, and no relative path exists between them).
    const std::filesystem::path previous = std::filesystem::current_path();
    std::filesystem::current_path(game.path().parent_path());

    {
        const Engine engine{{.mode = rtype::ecs::RunMode::kServer, .game = game.path().filename()}};

        EXPECT_TRUE(engine.gameDirectory().is_absolute());
        EXPECT_TRUE(std::filesystem::equivalent(engine.gameDirectory(), game.path()));
    }

    std::filesystem::current_path(previous);
}

TEST(Engine, RejectsAnInvalidCommandLine) {
    const GameDirectory game{"rtype-engine-bad-args"};

    EXPECT_THROW(Engine{serverConfig(game, {"server"})}, RuntimeError);
}

TEST(Engine, AddsThePluginsTheSettingsNameFromTheirLibraries) {
    const GameDirectory game{"rtype-engine-listed", R"(
        engine.settings.network.port = 0
        engine.settings.plugins = { "test.Loaded", "rtype.Physics" }
        engine.settings.physics.gravity = { 0, 0 }
    )"};
    Engine engine{serverConfig(game)};
    EXPECT_TRUE(engine.app().hasPlugin("test.Loaded"));
    EXPECT_TRUE(engine.app().hasPlugin("rtype.Physics"));
}

TEST(Engine, AddsTheCapsulePluginFromItsLibrary) {
    const GameDirectory game{"rtype-engine-capsules", R"(
        engine.settings.network.port = 0
        engine.settings.plugins = { "rtype.Capsule" }
    )"};
    Engine engine{serverConfig(game)};
    EXPECT_TRUE(engine.app().hasPlugin("rtype.Capsule"));
    EXPECT_TRUE(engine.app().getWorld().getResources().find("rtype.Capsules"));
    EXPECT_TRUE(engine.app().getWorld().getResources().find("rtype.LoadedCapsule"));
    EXPECT_NE(engine.app().getWorld().getCalls().findByName("rtype.capsule.spawn"), nullptr);
}

TEST(Engine, RefusesAPluginNoLibraryRegisters) {
    const GameDirectory game{"rtype-engine-unknown-plugin", R"(
        engine.settings.network.port = 0
        engine.settings.plugins = { "game.Missing" }
    )"};
    try {
        const Engine engine{serverConfig(game)};
        FAIL() << "the plugin was accepted";
    } catch (const RuntimeError& error) {
        EXPECT_TRUE(std::string{error.what()}.contains("game.Missing"));
    }
}

TEST(Engine, LoadsThePluginsOfTheGame) {
    const GameDirectory game{"rtype-engine-game-plugin", R"(
        engine.settings.network.port = 0
    )"};
    const std::filesystem::path plugins = game.path() / Engine::kPluginsDirectory;
    std::filesystem::create_directories(plugins);
    const std::filesystem::path library = rtype::core::system::getExecutableDirectory() / Engine::kPluginsDirectory;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator{library}) {
        if (entry.path().filename().string().contains("test-plugin")) {
            // A plugin of the same file name as one of the engine is refused, not silently preferred.
            std::filesystem::copy_file(std::filesystem::canonical(entry.path()), plugins / entry.path().filename());
        }
    }
    try {
        const Engine engine{serverConfig(game)};
        FAIL() << "the library was accepted";
    } catch (const RuntimeError& error) {
        EXPECT_TRUE(std::string{error.what()}.contains("same name"));
    }
}

TEST(Engine, LoadsTheBackendsAGameNames) {
    const GameDirectory game{"rtype-engine-backends", R"(
        engine.settings.platform = "glfw"
        engine.settings.renderer = "vulkan"
    )"};
    std::filesystem::create_directories(game.path() / Engine::kAssetsDirectory);
    const EngineConfig standalone{.mode = rtype::ecs::RunMode::kStandalone, .game = game.path()};
    EXPECT_NO_THROW(Engine{standalone});
}

TEST(Engine, GivesThePluginsTheAssetDirectory) {
    const GameDirectory game{"rtype-engine-assets"};
    Engine engine{serverConfig(game)};
    EXPECT_EQ(rtype::plugins::assetDirectoryOf(engine.app().getWorld().getResources()),
              engine.gameDirectory() / Engine::kAssetsDirectory);
}

TEST(Engine, AClientNeedsAnAssetsDirectoryAndAServerDoesNot) {
    const GameDirectory game{"rtype-engine-no-assets"};
    EngineConfig standalone{.mode = rtype::ecs::RunMode::kStandalone, .game = game.path()};
    EXPECT_THROW(Engine{standalone}, RuntimeError);
    EXPECT_NO_THROW(Engine{serverConfig(game)});

    std::filesystem::create_directories(game.path() / Engine::kAssetsDirectory);
    EXPECT_NO_THROW(Engine{standalone});
}

TEST(Engine, GivesTheProjectItsModeAndCommandLine) {
    const GameDirectory game{"rtype-engine-project", R"(
        engine.settings.network.port = 0
        engine.settings.seen.isServer = engine.isServer
        engine.settings.seen.isClient = engine.isClient
        engine.settings.seen.level = engine.args.level
    )"};

    const Engine engine{serverConfig(game, {"--level", "3"})};

    const rtype::core::config::Settings& settings = engine.settings();
    EXPECT_TRUE(settings.getBool("seen.isServer", false));
    EXPECT_FALSE(settings.getBool("seen.isClient", true));
    EXPECT_EQ(settings.getNumber("seen.level", 0.0), 3.0);
    EXPECT_EQ(engine.mode(), rtype::ecs::RunMode::kServer);
}

TEST(Engine, RejectsAPortOutOfRange) {
    const GameDirectory game{"rtype-engine-port-range"};

    EXPECT_THROW(Engine{serverConfig(game, {"--port", "65536"})}, RuntimeError);
    EXPECT_THROW(Engine{serverConfig(game, {"--port", "-1"})}, RuntimeError);
    EXPECT_THROW(Engine{serverConfig(game, {"--port", "80.5"})}, RuntimeError);
}

TEST(Engine, RejectsAnUnknownNetworkSetting) {
    const GameDirectory game{"rtype-engine-network-keys", R"(engine.settings.network.prot = 4242)"};

    EXPECT_THROW(Engine{serverConfig(game)}, rtype::core::config::exceptions::SettingsException);
}

TEST(Engine, RunsTheSetupBeforeTheScriptsAndTheNetwork) {
    const GameDirectory game{"rtype-engine-order"};
    bool replicationBefore = false;
    bool scriptingBefore = true;
    bool serverBefore = true;
    EngineConfig config = serverConfig(game);
    config.setup = [&](Engine& engine) {
        const rtype::ecs::App& app = engine.app();
        replicationBefore = app.hasPlugin(rtype::replication::ReplicationPlugin::kName);
        scriptingBefore = app.hasPlugin("rtype.scripting");
        serverBefore = app.hasPlugin(rtype::replication::server::ServerPlugin::kName);
    };

    const Engine engine{std::move(config)};

    EXPECT_TRUE(replicationBefore);  // so the game can declare its inputs
    EXPECT_FALSE(scriptingBefore);   // so the scripts can use what the game registers
    EXPECT_FALSE(serverBefore);      // so the server replicates what the game and the scripts register
    EXPECT_TRUE(engine.app().hasPlugin("rtype.scripting"));
    EXPECT_TRUE(engine.app().hasPlugin(rtype::replication::server::ServerPlugin::kName));
}

TEST(Engine, LoadsTheScriptsOfTheGame) {
    const GameDirectory game{"rtype-engine-scripts"};
    game.write("scripts/shield.luau", R"(engine.ecs.component("game.Shield", { fields = {} }))");

    const Engine engine{serverConfig(game)};

    EXPECT_TRUE(engine.app().getWorld().getComponents().find("game.Shield"));
}

TEST(Engine, RunsUntilAppExit) {
    const GameDirectory game{"rtype-engine-run"};
    std::int32_t builds = 0;
    EngineConfig config = serverConfig(game);
    config.setup = countingSetup(builds, &exitAtOnce);

    Engine engine{std::move(config)};

    EXPECT_EQ(engine.run(), 1);
    EXPECT_EQ(builds, 1);
}

TEST(Engine, ReloadsBetweenTwoFrames) {
    const GameDirectory game{"rtype-engine-reload"};
    std::int32_t builds = 0;
    EngineConfig config = serverConfig(game);
    config.setup = countingSetup(builds, &exitOnSecondBuild);
    Engine engine{std::move(config)};

    engine.requestReload();

    EXPECT_EQ(engine.run(), 2);  // the first build ran a frame, then the second one sent the exit
    EXPECT_EQ(builds, 2);
    EXPECT_EQ(engine.app().getWorld().getResources().require<Build>().number, 2);
}

TEST(Engine, ReadsTheProjectAgainOnAReload) {
    const GameDirectory game{"rtype-engine-reload-project", R"(
        engine.settings.network.port = 0
        engine.settings.version = 1
    )"};
    std::int32_t builds = 0;
    EngineConfig config = serverConfig(game);
    config.setup = countingSetup(builds, &exitOnSecondBuild);
    Engine engine{std::move(config)};
    game.write(std::string{Engine::kProjectFile}, R"(
        engine.settings.network.port = 0
        engine.settings.version = 2
    )");

    EXPECT_EQ(engine.settings().getNumber("version", 0.0), 1.0);
    engine.requestReload();
    EXPECT_EQ(engine.run(), 2);
    EXPECT_EQ(engine.settings().getNumber("version", 0.0), 2.0);
}

TEST(Engine, ExitsBeforeAReloadRequestedInTheSameFrame) {
    const GameDirectory game{"rtype-engine-exit-first"};
    std::int32_t builds = 0;
    EngineConfig config = serverConfig(game);
    config.setup = countingSetup(builds, &exitAtOnce);
    Engine engine{std::move(config)};

    engine.requestReload();

    EXPECT_EQ(engine.run(), 1);
    EXPECT_EQ(builds, 1);
}

namespace {
/// @brief The settings of the games that follow their files: hot reload is off by default in a release build.
constexpr std::string_view kHotProject = R"(
    engine.settings.network.port = 0
    engine.settings.hotReload = true
)";

/// @return A script whose system `game.Quit` ends the game with @p code, or does nothing if @p code is 0.
std::string quitScript(const int code) {
    return R"(engine.ecs.system("game.Quit", {
        phase = "Update",
        query = {},
        events = { send = { "rtype.AppExit" } },
        run = function(ctx) )" +
           (code == 0 ? std::string{} : "ctx.events.send(\"rtype.AppExit\", { code = " + std::to_string(code) + " })") +
           R"( end,
    }))";
}
}  // namespace

TEST(EngineHotReload, BuildsTheGameAgainWhenTheProjectChanges) {
    const GameDirectory game{"rtype-engine-hot-project", kHotProject};
    std::int32_t builds = 0;
    EngineConfig config = serverConfig(game);
    config.setup = countingSetup(builds, &exitOnSecondBuild);
    Engine engine{std::move(config)};
    game.write(std::string{Engine::kProjectFile}, std::string{kHotProject} + "engine.settings.version = 2\n");

    testing::internal::CaptureStderr();
    const int code = engine.run();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, 2);
    EXPECT_EQ(engine.settings().getNumber("version", 0.0), 2.0);
    EXPECT_NE(log.find("[HotReload] project: cannot apply in place"), std::string::npos) << log;
    EXPECT_NE(log.find("modified project.luau"), std::string::npos) << log;
}

TEST(EngineHotReload, SwapsTheSystemsOfAChangedScriptWithoutBuildingTheGameAgain) {
    const GameDirectory game{"rtype-engine-hot-scripts", kHotProject};
    game.write("scripts/quit.luau", quitScript(0));
    std::int32_t builds = 0;
    EngineConfig config = serverConfig(game);
    config.setup = [&builds](Engine& engine) {
        engine.app().insertResource(Build{.number = ++builds});
        static_cast<void>(engine.app().getWorld().getEvents().registerType<rtype::ecs::AppExit>());  // scripts send it
    };
    Engine engine{std::move(config)};
    game.write("scripts/quit.luau", quitScript(7));

    testing::internal::CaptureStderr();
    const int code = engine.run();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, 7) << "the new system ran in the same world";
    EXPECT_EQ(builds, 1) << "the game was not built again";
    EXPECT_NE(log.find("[HotReload] scripts: applied in"), std::string::npos) << log;
    EXPECT_NE(log.find("modified scripts/quit.luau"), std::string::npos) << log;
    EXPECT_NE(log.find("system game.Quit"), std::string::npos) << log;
}

TEST(EngineHotReload, BuildsTheGameAgainWhenAScriptCannotBeSwappedInPlace) {
    const GameDirectory game{"rtype-engine-hot-layout", kHotProject};
    game.write("scripts/thing.luau", R"(engine.ecs.component("game.Thing", { fields = { a = "f32" } }))");
    std::int32_t builds = 0;
    EngineConfig config = serverConfig(game);
    config.setup = countingSetup(builds, &exitOnSecondBuild);
    Engine engine{std::move(config)};
    game.write("scripts/thing.luau", R"(engine.ecs.component("game.Thing", { fields = { a = "f32", b = "f32" } }))");

    testing::internal::CaptureStderr();
    const int code = engine.run();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, 2);
    EXPECT_NE(log.find("[HotReload] scripts: cannot apply in place"), std::string::npos) << log;
    EXPECT_NE(log.find("game.Thing"), std::string::npos) << log;
}

TEST(EngineHotReload, WaitsForAFixWhenTheNewGameCannotBeBuilt) {
    const GameDirectory game{"rtype-engine-hot-wait", kHotProject};
    std::int32_t builds = 0;
    EngineConfig config = serverConfig(game);
    config.setup = countingSetup(builds, &exitOnSecondBuild);
    Engine engine{std::move(config)};
    game.write(std::string{Engine::kProjectFile},
               std::string{kHotProject} + "engine.settings.plugins = { 'no.Such' }\n");

    testing::internal::CaptureStderr();
    int code = -1;
    std::thread runner{[&engine, &code] { code = engine.run(); }};
    std::this_thread::sleep_for(std::chrono::milliseconds{1500});
    game.write(std::string{Engine::kProjectFile}, kHotProject);
    runner.join();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, 2) << "the game was built when the project was fixed";
    EXPECT_NE(log.find("The game cannot be built"), std::string::npos) << log;
    EXPECT_NE(log.find("unknown plugin 'no.Such'"), std::string::npos) << log;
    EXPECT_NE(log.find("Waiting for a change"), std::string::npos) << log;
}
