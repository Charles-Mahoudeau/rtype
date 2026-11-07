/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptRequireTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>

#include "ScriptsDirectory.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/ScriptHost.hpp"
#include "rtype/scripting/exceptions/ScriptLoadException.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

namespace {
using rtype::core::reload::ReloadOutcome;
using rtype::tests::ScriptsDirectory;
using Status = rtype::core::reload::ReloadOutcome::Status;

/// @brief An App with the scripting plugin over a scripts directory; the host is kept for the reloads.
struct Game {
    rtype::ecs::App app;
    rtype::scripting::ScriptHost* host{nullptr};

    explicit Game(const std::filesystem::path& directory) {
        app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}, rtype::ecs::FixedTimestep{1, 5}));
        app.addPlugin(rtype::scripting::ScriptingPlugin{
            {.directory = directory, .onBuilt = [this](rtype::scripting::ScriptHost& built) { host = &built; }}});
    }

    [[nodiscard]] bool hasComponent(const std::string_view name) const {
        return app.getWorld().getComponents().find(name).has_value();
    }

    [[nodiscard]] bool hasSystem(const std::string_view name) const {
        return app.getSchedules().getSchedule(rtype::ecs::Phase::kFixedUpdate).contains(name);
    }
};

/// @return A script declaring the system @p name, which does nothing.
std::string systemScript(const std::string_view name) {
    return "engine.ecs.system(\"" + std::string{name} +
           "\", { phase = \"FixedUpdate\", query = {}, run = function(ctx) end })\n";
}

/// @return The message of the exception building the game over @p directory throws, empty if it builds.
std::string buildError(const std::filesystem::path& directory) {
    try {
        const Game game{directory};
    } catch (const rtype::scripting::exceptions::ScriptLoadException& error) {
        return error.what();
    }
    return {};
}
}  // namespace

TEST(ScriptRequire, ASiblingRequireReturnsTheValueOfTheModule) {
    const ScriptsDirectory scripts{"rtype-require-sibling"};
    scripts.write("a_config.luau", "return { name = 'Configured' }");
    scripts.write("b_user.luau", R"(
        local config = require("./a_config")
        engine.ecs.component("game." .. config.name, { fields = {} })
    )");

    const Game game{scripts.path()};

    EXPECT_TRUE(game.hasComponent("game.Configured"));
}

TEST(ScriptRequire, AScriptsAliasRequiresFromTheRootOfScripts) {
    const ScriptsDirectory scripts{"rtype-require-alias"};
    scripts.write("lib/util.luau", "return { name = 'FromUtil' }");
    scripts.write("ui/panel.luau", R"(
        local util = require("@scripts/lib/util")
        engine.ecs.component("game." .. util.name, { fields = {} })
    )");

    const Game game{scripts.path()};

    EXPECT_TRUE(game.hasComponent("game.FromUtil"));
}

TEST(ScriptRequire, AFolderWithAnInitFileIsRequiredByItsName) {
    const ScriptsDirectory scripts{"rtype-require-init"};
    scripts.write("lib/init.luau", "return { name = 'FromInit' }");
    scripts.write("main.luau", R"(
        engine.ecs.component("game." .. require("@scripts/lib").name, { fields = {} })
    )");

    const Game game{scripts.path()};

    EXPECT_TRUE(game.hasComponent("game.FromInit"));
}

TEST(ScriptRequire, AModuleRequiredAndAutoLoadedRunsOnce) {
    const ScriptsDirectory scripts{"rtype-require-once"};
    // Loaded last by the auto-loading, but required before by both users.
    scripts.write("z_lib.luau", "Runs = (Runs or 0) + 1");
    scripts.write("a_user.luau", R"(require("./z_lib"))");
    scripts.write("b_user.luau", R"(require("./z_lib"))");
    scripts.write("c_check.luau", R"(
        assert(Runs == 1, "z_lib ran " .. tostring(Runs) .. " times")
        engine.ecs.component("game.Checked", { fields = {} })
    )");

    const Game game{scripts.path()};

    EXPECT_TRUE(game.hasComponent("game.Checked"));
}

TEST(ScriptRequire, AFileRequiredByAnotherIsNotRunAgainWhenItsTurnComes) {
    const ScriptsDirectory scripts{"rtype-require-skip"};
    scripts.write("a_first.luau", R"(require("./b_second"); Order = (Order or "") .. "a")");
    scripts.write("b_second.luau", R"(Order = (Order or "") .. "b")");
    scripts.write("c_check.luau", R"(
        assert(Order == "ba", "order was " .. tostring(Order))
        engine.ecs.component("game.Ordered", { fields = {} })
    )");

    const Game game{scripts.path()};

    EXPECT_TRUE(game.hasComponent("game.Ordered"));
}

TEST(ScriptRequire, GlobalsAreStillSharedAcrossFiles) {
    const ScriptsDirectory scripts{"rtype-require-globals"};
    scripts.write("a_sets.luau", "Duel = { rounds = 3 }");
    scripts.write("b_reads.luau", R"(
        assert(Duel.rounds == 3)
        engine.ecs.component("game.Shared", { fields = {} })
    )");

    const Game game{scripts.path()};

    EXPECT_TRUE(game.hasComponent("game.Shared"));
}

TEST(ScriptRequire, AReturnedTableIsFrozen) {
    const ScriptsDirectory scripts{"rtype-require-frozen"};
    scripts.write("a_lib.luau", "return { value = 1 }");
    scripts.write("b_user.luau", R"(
        local lib = require("./a_lib")
        assert(not pcall(function() lib.value = 2 end), "the table is not frozen")
        engine.ecs.component("game.Frozen", { fields = {} })
    )");

    const Game game{scripts.path()};

    EXPECT_TRUE(game.hasComponent("game.Frozen"));
}

TEST(ScriptRequire, ErrorsNameTheScriptsChunk) {
    const ScriptsDirectory scripts{"rtype-require-chunk"};
    scripts.write("nested/bad.luau", "\nerror('boom')");

    const std::string message = buildError(scripts.path());

    EXPECT_NE(message.find("scripts/nested/bad.luau:2: boom"), std::string::npos) << message;
}

TEST(ScriptRequire, ARequireErrorNamesTheRequiringFileAndLine) {
    const ScriptsDirectory scripts{"rtype-require-error"};
    scripts.write("a.luau", "local x = 1\nlocal y = require('./missing')");

    const std::string message = buildError(scripts.path());

    EXPECT_NE(message.find("Script 'scripts/a.luau' failed"), std::string::npos) << message;
    EXPECT_NE(message.find("scripts/a.luau:2: require(\"./missing\")"), std::string::npos) << message;
}

TEST(ScriptRequire, AModuleCannotRequireOutsideScripts) {
    const ScriptsDirectory scripts{"rtype-require-escape"};
    scripts.write("a.luau", "require('../../outside')");

    const std::string message = buildError(scripts.path());

    EXPECT_NE(message.find("escapes scripts/"), std::string::npos) << message;
}

TEST(ScriptRequire, ADeclarationBelongsToTheModuleThatMadeIt) {
    const ScriptsDirectory scripts{"rtype-require-owner"};
    scripts.write("a_lib.luau", systemScript("game.FromLib"));
    scripts.write("b_main.luau", "require('./a_lib')\n" + systemScript("game.FromMain"));
    Game game{scripts.path()};
    ASSERT_TRUE(game.hasSystem("game.FromLib"));
    ASSERT_TRUE(game.hasSystem("game.FromMain"));

    // Reloading the requirer replaces its own system only: the required module keeps what it declared.
    const ReloadOutcome main =
        game.host->reloadScript("scripts/b_main.luau", "require('./a_lib')\n" + systemScript("game.MainAgain"));

    EXPECT_EQ(main.status, Status::kApplied) << main.message;
    EXPECT_TRUE(game.hasSystem("game.FromLib"));
    EXPECT_FALSE(game.hasSystem("game.FromMain"));
    EXPECT_TRUE(game.hasSystem("game.MainAgain"));

    // And the other way round.
    const ReloadOutcome lib = game.host->reloadScript("scripts/a_lib.luau", systemScript("game.LibAgain"));

    EXPECT_EQ(lib.status, Status::kApplied) << lib.message;
    EXPECT_FALSE(game.hasSystem("game.FromLib"));
    EXPECT_TRUE(game.hasSystem("game.LibAgain"));
    EXPECT_TRUE(game.hasSystem("game.MainAgain"));
}

TEST(ScriptRequire, ReloadingAModuleGivesTheNewValueToLaterRequires) {
    const ScriptsDirectory scripts{"rtype-require-reload"};
    scripts.write("a_lib.luau", "return { n = 1 }");
    const std::string user =
        "engine.ecs.system('game.Val' .. require('./a_lib').n, "
        "{ phase = 'FixedUpdate', query = {}, run = function(ctx) end })";
    scripts.write("b_user.luau", user);
    Game game{scripts.path()};
    ASSERT_TRUE(game.hasSystem("game.Val1"));

    EXPECT_EQ(game.host->reloadScript("scripts/a_lib.luau", "return { n = 2 }").status, Status::kApplied);
    // The requirer is not re-run by the reload of its module (that comes later), only when it reloads itself.
    EXPECT_TRUE(game.hasSystem("game.Val1"));
    EXPECT_EQ(game.host->reloadScript("scripts/b_user.luau", user).status, Status::kApplied);

    EXPECT_TRUE(game.hasSystem("game.Val2"));
    EXPECT_FALSE(game.hasSystem("game.Val1"));
}

TEST(ScriptRequire, AFailedReloadOfAModuleKeepsTheOldValue) {
    const ScriptsDirectory scripts{"rtype-require-rollback"};
    scripts.write("a_lib.luau", "return { n = 1 }");
    const std::string user =
        "engine.ecs.system('game.Val' .. require('./a_lib').n, "
        "{ phase = 'FixedUpdate', query = {}, run = function(ctx) end })";
    scripts.write("b_user.luau", user);
    Game game{scripts.path()};

    const ReloadOutcome failed = game.host->reloadScript("scripts/a_lib.luau", "error('nope')");

    EXPECT_EQ(failed.status, Status::kFailed);
    EXPECT_EQ(game.host->reloadScript("scripts/b_user.luau", user).status, Status::kApplied);
    EXPECT_TRUE(game.hasSystem("game.Val1"));
}

TEST(ScriptRequire, AHostWithoutAResolverRunsScriptsWithoutRequire) {
    rtype::ecs::App app;
    auto host = rtype::scripting::ScriptHost::create(app, {});
    ASSERT_TRUE(host);

    EXPECT_TRUE((*host)->run("plain.luau", "Answer = 42"));
    EXPECT_FALSE((*host)->run("needs.luau", "require('./x')"));
    EXPECT_FALSE((*host)->require("./x"));
}
