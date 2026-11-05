/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EngineTests
*/

#include <gtest/gtest.h>

#include <cstdint>
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
    const std::filesystem::path relative = std::filesystem::relative(game.path());

    const Engine engine{{.mode = rtype::ecs::RunMode::kServer, .game = relative}};

    EXPECT_TRUE(engine.gameDirectory().is_absolute());
    EXPECT_TRUE(std::filesystem::equivalent(engine.gameDirectory(), game.path()));
}

TEST(Engine, RejectsAnInvalidCommandLine) {
    const GameDirectory game{"rtype-engine-bad-args"};

    EXPECT_THROW(Engine{serverConfig(game, {"server"})}, RuntimeError);
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
