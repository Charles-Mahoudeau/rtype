/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EngineTests
*/

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "core/system/ExecutablePath.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
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

TEST(Engine, AddsTheExtraPluginsAfterTheListedOnes) {
    const GameDirectory game{"rtype-engine-extra", R"(
        engine.settings.network.port = 0
        engine.settings.plugins = { "test.Loaded" }
    )"};
    EngineConfig config = serverConfig(game);
    config.extraPlugins = {"rtype.Capsule"};
    Engine engine{std::move(config)};
    EXPECT_TRUE(engine.app().hasPlugin("test.Loaded"));
    EXPECT_TRUE(engine.app().hasPlugin("rtype.Capsule"));
}

TEST(Engine, DoesNotAddAnExtraPluginTheProjectAlreadyLists) {
    const GameDirectory game{"rtype-engine-extra-listed", R"(
        engine.settings.network.port = 0
        engine.settings.plugins = { "rtype.Capsule" }
    )"};
    EngineConfig config = serverConfig(game);
    config.extraPlugins = {"rtype.Capsule"};
    const Engine engine{std::move(config)};
    EXPECT_TRUE(engine.app().hasPlugin("rtype.Capsule"));
}

TEST(Engine, RefusesAnExtraPluginNoLibraryRegisters) {
    const GameDirectory game{"rtype-engine-extra-missing", R"(
        engine.settings.network.port = 0
    )"};
    EngineConfig config = serverConfig(game);
    config.extraPlugins = {"game.Missing"};
    try {
        const Engine engine{std::move(config)};
        FAIL() << "the plugin was accepted";
    } catch (const RuntimeError& error) {
        EXPECT_TRUE(std::string{error.what()}.contains("game.Missing"));
    }
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

TEST(Engine, ReadsTheGamesCapsuleFilesAfterItsScripts) {
    const GameDirectory game{"rtype-engine-capsule-files", R"(
        engine.settings.network.port = 0
        engine.settings.plugins = { "rtype.Capsule" }
    )"};
    // The file names a component a script declares: it is read once every plugin, scripts included, is built.
    game.write("assets/actors/bydo.actor", "[node path=\".\"]\ngame.Health = { value = 3 }\n");
    game.write("scripts/main.luau", R"(
        engine.ecs.component("game.Health", { fields = { value = "i32" } })
        local loaded = false
        engine.ecs.system("game.Start", {
            phase = "Update",
            query = {},
            calls = { "rtype.capsule.load" },
            run = function(ctx)
                if loaded then return end
                loaded = true
                ctx.call("rtype.capsule.load", { capsule = "actors/bydo.actor" })
            end,
        })
    )");
    Engine engine{serverConfig(game)};
    engine.app().runFrame();
    const rtype::ecs::World& world = engine.app().getWorld();
    const auto health = world.getComponents().find("game.Health");
    ASSERT_TRUE(health);
    const auto* loaded = world.getResources().get<rtype::plugins::capsule::LoadedCapsule>();
    ASSERT_NE(loaded, nullptr);
    ASSERT_TRUE(world.isAlive(loaded->root));
    const std::span<const std::byte> value = world.getComponents().get(loaded->root, *health);
    ASSERT_EQ(value.size(), sizeof(std::int32_t));
    std::int32_t points = 0;
    std::memcpy(&points, value.data(), sizeof(points));
    EXPECT_EQ(points, 3);
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
    game.write("scripts/main.luau", R"(engine.ecs.component("game.Shield", { fields = {} }))");

    const Engine engine{serverConfig(game)};

    EXPECT_TRUE(engine.app().getWorld().getComponents().find("game.Shield"));
}

TEST(Engine, TheScriptsOfAGameRequireEachOtherThroughTheScriptsAlias) {
    const GameDirectory game{"rtype-engine-require"};
    game.write("scripts/shared/config.luau", "return { name = 'Configured' }");
    game.write("scripts/main.luau", R"(
        local config = require("@scripts/shared/config")
        engine.ecs.component("game." .. config.name, { fields = {} })
    )");

    const Engine engine{serverConfig(game)};

    EXPECT_TRUE(engine.app().getWorld().getComponents().find("game.Configured"));
}

TEST(Engine, ScriptsRequireThroughAnAliasTheSettingsDeclare) {
    const GameDirectory game{"rtype-engine-alias", R"(
        engine.settings.network.port = 0
        engine.settings.scripts.aliases = { shared = "shared" }
    )"};
    game.write("scripts/shared/x.luau", "return { name = 'Aliased' }");
    game.write("scripts/main.luau", R"(
        local x = require("@shared/x")
        engine.ecs.component("game." .. x.name, { fields = {} })
    )");

    const Engine engine{serverConfig(game)};

    EXPECT_TRUE(engine.app().getWorld().getComponents().find("game.Aliased"));
}

/// @return What building a game whose project is @p project throws, empty if nothing.
std::string buildError(const std::string_view name, const std::string_view project,
                       const rtype::ecs::RunMode mode = rtype::ecs::RunMode::kServer) {
    const GameDirectory game{name, project};
    game.write("scripts/main.luau", "return 1");
    try {
        EngineConfig config = serverConfig(game);
        config.mode = mode;
        const Engine engine{std::move(config)};
    } catch (const std::exception& error) {
        return error.what();
    }
    return {};
}

TEST(Engine, EntryComesFromTheProjectPerSide) {
    constexpr std::string_view kEntries = R"(
        engine.settings.network.port = 0
        engine.settings.scripts.main = "shared/main"
        engine.settings.scripts.server = "server/main"
        engine.settings.scripts.client = "client/main"
    )";
    const auto build = [&](const std::string_view name, const rtype::ecs::RunMode mode) {
        const GameDirectory game{name, kEntries};
        game.write("scripts/shared/main.luau", R"(engine.ecs.component("game.Main", { fields = {} }))");
        game.write("scripts/server/main.luau", R"(engine.ecs.component("game.Server", { fields = {} }))");
        game.write("scripts/client/main.luau", R"(engine.ecs.component("game.Client", { fields = {} }))");
        game.write("assets/placeholder.txt", "");
        EngineConfig config = serverConfig(game);
        config.mode = mode;
        const Engine engine{std::move(config)};
        const auto& components = engine.app().getWorld().getComponents();
        return std::array{components.find("game.Main").has_value(), components.find("game.Server").has_value(),
                          components.find("game.Client").has_value()};
    };

    EXPECT_EQ(build("rtype-engine-entry-server", rtype::ecs::RunMode::kServer), (std::array{true, true, false}));
    EXPECT_EQ(build("rtype-engine-entry-standalone", rtype::ecs::RunMode::kStandalone), (std::array{true, true, true}));
}

TEST(Engine, AMissingExplicitEntryFailsTheBuildNamingItsSetting) {
    const std::string message = buildError("rtype-engine-entry-missing", R"(
        engine.settings.network.port = 0
        engine.settings.scripts.server = "server/main"
    )");
    EXPECT_NE(message.find("scripts.server"), std::string::npos) << message;
}

TEST(Engine, AMissingDefaultEntryOnlyWarns) {
    const GameDirectory game{"rtype-engine-entry-default"};

    testing::internal::CaptureStderr();
    const Engine engine{serverConfig(game)};
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("No entry script 'main'"), std::string::npos) << log;
}

TEST(Engine, RejectsAScriptsAliasOutsideTheScripts) {
    const std::string message = buildError("rtype-engine-alias-outside", R"(
        engine.settings.network.port = 0
        engine.settings.scripts.aliases = { escape = "../assets" }
    )");
    EXPECT_NE(message.find("scripts.aliases.escape"), std::string::npos) << message;
    EXPECT_NE(message.find("inside scripts/"), std::string::npos) << message;
}

TEST(Engine, RejectsAReservedScriptsAlias) {
    const std::string message = buildError("rtype-engine-alias-reserved", R"(
        engine.settings.network.port = 0
        engine.settings.scripts.aliases = { self = "shared" }
    )");
    EXPECT_NE(message.find("scripts.aliases.self"), std::string::npos) << message;
    EXPECT_NE(message.find("reserved"), std::string::npos) << message;
}

TEST(Engine, RejectsAScriptsAliasThatIsNotAString) {
    const std::string message = buildError("rtype-engine-alias-type", R"(
        engine.settings.network.port = 0
        engine.settings.scripts.aliases = { shared = 3 }
    )");
    EXPECT_NE(message.find("scripts.aliases.shared"), std::string::npos) << message;
    EXPECT_NE(message.find("string"), std::string::npos) << message;
}

TEST(Engine, RejectsAnUnknownScriptsSetting) {
    const std::string message = buildError("rtype-engine-scripts-key", R"(
        engine.settings.network.port = 0
        engine.settings.scripts.alias = { shared = "shared" }
    )");
    EXPECT_NE(message.find("alias.shared"), std::string::npos) << message;
    EXPECT_NE(message.find("'scripts'"), std::string::npos) << message;
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
    game.write("scripts/main.luau", "require('./quit')");
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

namespace {
/// @return A script whose system @p name ends the game with the code of the module `config`, if it is not 0.
std::string configuredQuitScript(const std::string_view name) {
    return "local config = require('./config')\nengine.ecs.system(\"" + std::string{name} + R"(", {
        phase = "Update",
        query = {},
        events = { send = { "rtype.AppExit" } },
        run = function(ctx)
            if config.code ~= 0 then ctx.events.send("rtype.AppExit", { code = config.code }) end
        end,
    }))";
}

/// @return A script whose system `game.Other` ends the game with @p code, or does nothing if @p code is 0.
std::string otherScript(const int code) {
    std::string script = quitScript(code);
    script.replace(script.find("game.Quit"), std::string_view{"game.Quit"}.size(), "game.Other");
    // Both send the exit event: an order between them is required.
    constexpr std::string_view kPhase = "phase = \"Update\",";
    script.replace(script.find(kPhase), kPhase.size(), "phase = \"Update\", after = { \"game.Quit\" },");
    return script;
}

/// @brief Setup that registers the exit event scripts send.
void registerExit(Engine& engine) {
    static_cast<void>(engine.app().getWorld().getEvents().registerType<rtype::ecs::AppExit>());
}
}  // namespace

TEST(EngineHotReload, ReRunsTheDependentsOfAChangedModule) {
    const GameDirectory game{"rtype-engine-hot-dependents", kHotProject};
    game.write("scripts/main.luau", "require('./quit')");
    game.write("scripts/config.luau", "return { code = 0 }");
    game.write("scripts/quit.luau", configuredQuitScript("game.Quit"));
    EngineConfig config = serverConfig(game);
    config.setup = &registerExit;
    Engine engine{std::move(config)};
    game.write("scripts/config.luau", "return { code = 9 }");

    testing::internal::CaptureStderr();
    const int code = engine.run();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, 9) << "quit.luau ran again and saw the new value of config.luau";
    EXPECT_NE(log.find("[HotReload] scripts: applied in"), std::string::npos) << log;
    EXPECT_NE(log.find("system game.Quit"), std::string::npos) << log;
}

TEST(EngineHotReload, DoesNotReRunTheModulesThatDoNotRequireTheChangedOne) {
    const GameDirectory game{"rtype-engine-hot-unrelated", kHotProject};
    game.write("scripts/main.luau", "require('./quit')\nrequire('./other')");
    game.write("scripts/config.luau", "return { code = 0 }");
    game.write("scripts/quit.luau", configuredQuitScript("game.Quit"));
    game.write("scripts/other.luau", otherScript(0));
    EngineConfig config = serverConfig(game);
    config.setup = &registerExit;
    Engine engine{std::move(config)};
    game.write("scripts/other.luau", otherScript(5));

    testing::internal::CaptureStderr();
    const int code = engine.run();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, 5) << log;
    EXPECT_NE(log.find("system game.Other"), std::string::npos) << log;
    EXPECT_EQ(log.find("system game.Quit"), std::string::npos) << log;
}

TEST(EngineHotReload, IgnoresAChangedFileNoModuleRequires) {
    const GameDirectory game{"rtype-engine-hot-ignored", kHotProject};
    game.write("scripts/main.luau", "require('./quit')");
    game.write("scripts/quit.luau", quitScript(0));
    game.write("scripts/unused.luau", "return 1");
    EngineConfig config = serverConfig(game);
    config.setup = &registerExit;
    Engine engine{std::move(config)};
    // Both are saved in the same batch: the unused file would fail the reload if it were run.
    game.write("scripts/unused.luau", "error('never run')");
    game.write("scripts/quit.luau", quitScript(3));

    testing::internal::CaptureStderr();
    const int code = engine.run();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, 3) << log;
    EXPECT_EQ(log.find("never run"), std::string::npos) << log;
}

TEST(EngineHotReload, BuildsTheGameAgainWhenAScriptCannotBeSwappedInPlace) {
    const GameDirectory game{"rtype-engine-hot-layout", kHotProject};
    game.write("scripts/main.luau", "require('./thing')");
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

TEST(EngineHotReload, RespawnsWhatACapsuleFileSpawnedWhenItIsSaved) {
    const GameDirectory game{"rtype-engine-hot-capsule",
                             std::string{kHotProject} + "engine.settings.plugins = { \"rtype.Capsule\" }\n"};
    game.write("assets/actors/bydo.actor", "[node path=\".\"]\ngame.Health = { value = 3 }\n");
    game.write("scripts/main.luau", R"(
        engine.ecs.component("game.Health", { fields = { value = "i32" } })
        local loaded = false
        engine.ecs.system("game.Start", {
            phase = "Update",
            query = {},
            calls = { "rtype.capsule.load" },
            run = function(ctx)
                if loaded then return end
                loaded = true
                ctx.call("rtype.capsule.load", { capsule = "actors/bydo.actor" })
            end,
        })
        engine.ecs.system("game.Quit", {
            phase = "Update",
            query = { read = { "game.Health" } },
            events = { send = { "rtype.AppExit" } },
            run = function(ctx)
                for _, health in ctx.query do
                    if health.value == 42 then
                        ctx.events.send("rtype.AppExit", { code = 42 })
                    end
                end
            end,
        })
    )");
    EngineConfig config = serverConfig(game);
    config.setup = [](Engine& engine) {
        static_cast<void>(engine.app().getWorld().getEvents().registerType<rtype::ecs::AppExit>());  // scripts send it
    };
    Engine engine{std::move(config)};
    engine.app().runFrame();  // loads the capsule
    game.write("assets/actors/bydo.actor", "[node path=\".\"]\ngame.Health = { value = 42 }\n");

    testing::internal::CaptureStderr();
    const int code = engine.run();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(code, 42) << "the loaded capsule was respawned with the saved value";
    EXPECT_NE(log.find("[HotReload] capsules: applied in"), std::string::npos) << log;
    EXPECT_NE(log.find("modified assets/actors/bydo.actor"), std::string::npos) << log;
    EXPECT_NE(log.find("capsule actors/bydo.actor"), std::string::npos) << log;
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
