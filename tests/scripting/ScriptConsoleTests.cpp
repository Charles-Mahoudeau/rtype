/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptConsoleTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "ScriptsDirectory.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/ScriptHost.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

namespace {
using rtype::tests::ScriptsDirectory;

/// @brief A game over a scripts directory, and the host of its scripts, as the editor's console sees them.
struct Game {
    rtype::ecs::App app;
    rtype::scripting::ScriptHost* host{nullptr};

    explicit Game(const std::filesystem::path& directory) {
        app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}, rtype::ecs::FixedTimestep{1, 5}));
        app.addPlugin(rtype::scripting::ScriptingPlugin{
            {.directory = directory, .onBuilt = [this](rtype::scripting::ScriptHost& built) { host = &built; }}});
    }

    /// @return What the console gives for @p line; `error: ` and the message if it failed.
    [[nodiscard]] std::string eval(const std::string_view line) const {
        const auto evaluation = host->evaluate(line);
        return evaluation.ok ? evaluation.text : "error: " + evaluation.text;
    }
};

/// @brief The scripts of a game with a config the console edits: `shared/config` and `probe`, a system checking the
/// config stays read-only for the game.
void writeGame(const ScriptsDirectory& scripts) {
    scripts.write("shared/config.luau", "return { speed = 4, name = 'Ship', player = { size = { x = 1, y = 2 } } }");
    scripts.write("probe.luau", "return { log = {} }");
    scripts.write("main.luau", R"(
        local config = require("./shared/config")
        local probe = require("./probe")
        engine.ecs.system("probe.check", {
            phase = "FixedUpdate", query = {},
            run = function(ctx)
                probe.log.rejected = not pcall(function() config.speed = 99 end)
                probe.log.speed = config.speed
                probe.log.x = config.player.size.x
            end,
        })
    )");
}
}  // namespace

TEST(ScriptConsole, WritesTheValueOfAnExpression) {
    const ScriptsDirectory scripts{"rtype-console-expression"};
    writeGame(scripts);
    const Game game{scripts.path()};

    EXPECT_EQ(game.eval("config.speed"), "4");
    EXPECT_EQ(game.eval("config.name"), "\"Ship\"");
    EXPECT_EQ(game.eval("config.player.size"), "{ x = 1, y = 2 }");
    EXPECT_EQ(game.eval("1 + 1, 'two'"), "2, \"two\"");
    EXPECT_EQ(game.eval("config.missing"), "nil");
}

TEST(ScriptConsole, RunsAStatementAndGivesNothing) {
    const ScriptsDirectory scripts{"rtype-console-statement"};
    writeGame(scripts);
    const Game game{scripts.path()};

    EXPECT_EQ(game.eval("config.player.size.x = 10"), "");
    EXPECT_EQ(game.eval("config.player.size.x"), "10");
}

TEST(ScriptConsole, EditsTheTopLevelOfAModuleThenFreezesItAgain) {
    const ScriptsDirectory scripts{"rtype-console-freeze"};
    writeGame(scripts);
    Game game{scripts.path()};

    EXPECT_EQ(game.eval("config.speed = 7; config.extra = true"), "");
    game.app.runFrame();

    // The game sees the new values at once, and still cannot write the table.
    EXPECT_EQ(game.eval("config.speed, config.extra"), "7, true");
    EXPECT_EQ(game.eval("probe.log.speed"), "7");
    EXPECT_EQ(game.eval("probe.log.rejected"), "true");
}

TEST(ScriptConsole, TheGameReadsWhatTheConsoleWroteInANestedTable) {
    const ScriptsDirectory scripts{"rtype-console-nested"};
    writeGame(scripts);
    Game game{scripts.path()};

    ASSERT_EQ(game.eval("config.player.size.x = 10"), "");
    game.app.runFrame();

    EXPECT_EQ(game.eval("probe.log.x"), "10");
}

TEST(ScriptConsole, ReachesModulesByKeyAndByWholeName) {
    const ScriptsDirectory scripts{"rtype-console-names"};
    writeGame(scripts);
    scripts.write("other/config.luau", "return { speed = 1 }");
    scripts.write("main.luau", R"(
        require("./shared/config")
        require("./other/config")
    )");
    const Game game{scripts.path()};

    EXPECT_EQ(game.eval("modules['shared/config'].speed"), "4");
    EXPECT_EQ(game.eval("modules['other/config'].speed"), "1");
    // One of the two takes the short name, the other the whole key.
    const std::string shared = game.eval("shared_config and shared_config.speed");
    const std::string other = game.eval("other_config and other_config.speed");
    EXPECT_TRUE(shared == "4" || other == "1") << shared << " / " << other;
}

TEST(ScriptConsole, ReportsACompilationErrorAndARuntimeError) {
    const ScriptsDirectory scripts{"rtype-console-errors"};
    writeGame(scripts);
    const Game game{scripts.path()};

    EXPECT_TRUE(game.eval("config.speed = ").starts_with("error: "));
    EXPECT_TRUE(game.eval("config.nothing.here").starts_with("error: "));
    EXPECT_TRUE(game.eval("error('boom', 0)").find("boom") != std::string::npos);
    EXPECT_EQ(game.eval("config.speed"), "4");  // the console still works
}

TEST(ScriptConsole, CannotDeclareOnTheScheduleButCanCallTheEngine) {
    const ScriptsDirectory scripts{"rtype-console-engine"};
    writeGame(scripts);
    const Game game{scripts.path()};

    EXPECT_TRUE(game.eval("engine.ecs").starts_with("nil") || game.eval("engine.ecs") == "nil");
    EXPECT_EQ(game.eval("engine.isClient"), "false");
    EXPECT_EQ(game.eval("engine.hasCall('does.not.exist')"), "false");
}

TEST(ScriptConsole, CompletesNamesAmongModulesFieldsAndGlobals) {
    const ScriptsDirectory scripts{"rtype-console-complete"};
    writeGame(scripts);
    const Game game{scripts.path()};

    EXPECT_EQ(game.host->complete("config.pl"), (std::vector<std::string>{"config.player"}));
    EXPECT_EQ(game.host->complete("config.player.size."),
              (std::vector<std::string>{"config.player.size.x", "config.player.size.y"}));
    EXPECT_EQ(game.host->complete("conf"), (std::vector<std::string>{"config"}));
    const std::vector<std::string> globals = game.host->complete("mat");
    EXPECT_NE(std::ranges::find(globals, "math"), globals.end());
    EXPECT_TRUE(game.host->complete("config.zzz").empty());
    EXPECT_TRUE(game.host->complete("nothing.at").empty());
    EXPECT_TRUE(game.host->complete("config.speed.").empty());
}

TEST(ScriptConsole, DoesNotCompleteAnEcsDeclarationOfEngine) {
    const ScriptsDirectory scripts{"rtype-console-engine-complete"};
    writeGame(scripts);
    const Game game{scripts.path()};

    const std::vector<std::string> words = game.host->complete("engine.");

    EXPECT_EQ(std::ranges::find(words, "engine.ecs"), words.end());
    EXPECT_NE(std::ranges::find(words, "engine.call"), words.end());
}

TEST(ScriptConsole, FailsBeforeAnyScriptRan) {
    rtype::ecs::App app;
    auto host = rtype::scripting::ScriptHost::create(app, {});
    ASSERT_TRUE(host);

    const auto evaluation = (*host)->evaluate("1 + 1");

    EXPECT_FALSE(evaluation.ok);
    EXPECT_TRUE((*host)->complete("ma").empty());
}
