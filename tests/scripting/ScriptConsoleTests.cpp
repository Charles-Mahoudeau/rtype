/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptConsoleTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <format>
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

    /// @brief Gives a new entity the component @p name, with its defaults.
    rtype::ecs::Entity spawn(const std::string_view name) {
        const auto* layout = host->findComponent(name);
        EXPECT_NE(layout, nullptr);
        const rtype::ecs::Entity entity = app.getWorld().spawn();
        EXPECT_TRUE(app.getWorld().getComponents().add(entity, layout->id, layout->defaults));
        return entity;
    }

    /// @return What the console gives for @p line; `error: ` and the message if it failed.
    [[nodiscard]] std::string eval(const std::string_view line) const {
        const auto evaluation = host->evaluate(line);
        return evaluation.ok ? evaluation.text : "error: " + evaluation.text;
    }

    /// @return What the console gives for @p line with `self` standing for @p self.
    [[nodiscard]] std::string eval(const std::string_view line, const rtype::ecs::Entity self,
                                   const bool writable = true) const {
        const auto evaluation = host->evaluate(line, {.self = self, .writable = writable});
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

namespace {
/// @brief A game that declares two components, `game.Ship` and `other.Ship`, and an `game.Engine` tag.
void writeShipGame(const ScriptsDirectory& scripts) {
    scripts.write("probe.luau", "return { log = {} }");
    scripts.write("main.luau", R"(
        require("./probe")
        engine.ecs.component("game.Ship", { fields = { speed = "f32", hp = "u32", position = "vec2", name = "string" } })
        engine.ecs.component("other.Ship", { fields = { size = "f32" } })
        engine.ecs.component("game.Tag", { fields = { } })
    )");
}
}  // namespace

TEST(ScriptConsoleEntities, ReadsAndWritesTheFieldsOfTheSelectedEntityInPlace) {
    const ScriptsDirectory scripts{"rtype-console-entity"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Ship");

    EXPECT_EQ(game.eval("self.Ship.speed", ship), "0");
    EXPECT_EQ(game.eval("self['game.Ship'].speed = 3.5; self['game.Ship'].hp = 7", ship), "");
    EXPECT_EQ(game.eval("self['game.Ship'].speed, self['game.Ship'].hp", ship), "3.5, 7");
    const auto* layout = game.host->findComponent("game.Ship");
    const auto bytes = game.app.getWorld().getComponents().get(ship, layout->id);
    float speed = 0.F;
    const auto field = std::ranges::find(layout->fields, "speed", &rtype::ecs::FieldInfo::name);
    ASSERT_NE(field, layout->fields.end());
    std::memcpy(&speed, bytes.data() + field->offset, sizeof(float));
    EXPECT_FLOAT_EQ(speed, 3.5F);
}

TEST(ScriptConsoleEntities, MarksTheComponentChanged) {
    const ScriptsDirectory scripts{"rtype-console-changed"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Ship");
    const auto layout = game.host->findComponent("game.Ship")->id;
    ASSERT_EQ(game.eval("self['game.Ship'].speed = 1", ship), "");

    const auto after = game.app.getWorld().getComponents().getTicks(ship, layout);
    ASSERT_TRUE(after);
    EXPECT_EQ(after->changed, game.app.getWorld().getComponents().getChangeTick());
}

TEST(ScriptConsoleEntities, SelfIsNilWithoutAnEntityOrWithADeadOne) {
    const ScriptsDirectory scripts{"rtype-console-self-nil"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Ship");
    ASSERT_TRUE(game.app.getWorld().despawn(ship));

    EXPECT_EQ(game.eval("self"), "nil");
    EXPECT_EQ(game.eval("self", ship), "nil");
}

TEST(ScriptConsoleEntities, ReadsButDoesNotWriteWhenTheConsoleOnlyReads) {
    const ScriptsDirectory scripts{"rtype-console-readonly"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Ship");

    EXPECT_EQ(game.eval("self['game.Ship'].speed", ship, false), "0");
    EXPECT_TRUE(game.eval("self['game.Ship'].speed = 9", ship, false).starts_with("error: "));
    EXPECT_EQ(game.eval("self['game.Ship'].speed", ship), "0");
}

TEST(ScriptConsoleEntities, FindsAComponentByItsLastPartAndSaysWhenItIsAmbiguous) {
    const ScriptsDirectory scripts{"rtype-console-names-ecs"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Ship");  // has game.Ship only

    // `Ship` is both game.Ship and other.Ship: the entity has one of them, which settles it.
    EXPECT_EQ(game.eval("self.Ship.speed", ship), "0");
    const auto both = game.spawn("game.Ship");
    ASSERT_TRUE(game.app.getWorld().getComponents().add(both, game.host->findComponent("other.Ship")->id,
                                                        game.host->findComponent("other.Ship")->defaults));
    const std::string ambiguous = game.eval("self.Ship", both);
    EXPECT_TRUE(ambiguous.starts_with("error: ")) << ambiguous;
    EXPECT_NE(ambiguous.find("game.Ship"), std::string::npos);
    EXPECT_NE(ambiguous.find("other.Ship"), std::string::npos);
    EXPECT_EQ(game.eval("self['other.Ship'].size", both), "0");
}

TEST(ScriptConsoleEntities, GivesNilForAMissingComponentAndAnErrorForAnUnknownOne) {
    const ScriptsDirectory scripts{"rtype-console-missing"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto tagged = game.spawn("game.Tag");

    EXPECT_EQ(game.eval("self['game.Ship']", tagged), "nil");
    EXPECT_TRUE(game.eval("self.Nothing", tagged).find("no component named 'Nothing'") != std::string::npos);
    EXPECT_TRUE(game.eval("self['game.Ship'].nothing", game.spawn("game.Ship")).find("no field 'nothing'") !=
                std::string::npos);
}

TEST(ScriptConsoleEntities, RefusesAValueOfTheWrongType) {
    const ScriptsDirectory scripts{"rtype-console-wrong-type"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Ship");

    EXPECT_TRUE(game.eval("self['game.Ship'].speed = 'fast'", ship).starts_with("error: "));
    EXPECT_EQ(game.eval("self['game.Ship'].speed", ship), "0");
}

TEST(ScriptConsoleEntities, ReachesAnotherEntityByIndexAndGeneration) {
    const ScriptsDirectory scripts{"rtype-console-entity-fn"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Ship");
    const std::string call = std::format("entity({}, {})", ship.getIndex(), ship.getGeneration());
    const std::string bits = std::format("entity({})", ship.getBits());

    EXPECT_EQ(game.eval(call + "['game.Ship'].hp = 4"), "");
    EXPECT_EQ(game.eval(bits + "['game.Ship'].hp"), "4");
    EXPECT_TRUE(game.eval("entity(999, 1)").starts_with("error: "));
    EXPECT_TRUE(game.eval("entity('x')").starts_with("error: "));
}

TEST(ScriptConsoleEntities, AHandleDoesNotOutliveItsLine) {
    const ScriptsDirectory scripts{"rtype-console-stale"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Ship");

    ASSERT_EQ(game.eval("probe.log.kept = self['game.Ship']", ship), "");

    const std::string later = game.eval("probe.log.kept.speed");
    EXPECT_TRUE(later.starts_with("error: ")) << later;
    EXPECT_NE(later.find("no longer valid"), std::string::npos);
}

TEST(ScriptConsoleEntities, CompletesTheComponentsOfSelfAndTheirFields) {
    const ScriptsDirectory scripts{"rtype-console-complete-entity"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Tag");
    ASSERT_TRUE(game.app.getWorld().getComponents().add(ship, game.host->findComponent("game.Ship")->id,
                                                        game.host->findComponent("game.Ship")->defaults));
    // `Ship` is two components, but the entity has one: it is not a name a dot can reach, `Tag` is.
    const std::vector<std::string> components = game.host->complete("self.", {.self = ship});
    EXPECT_NE(std::ranges::find(components, "self.Tag"), components.end());

    const auto lone = game.spawn("game.Ship");
    const std::vector<std::string> none = game.host->complete("self.Ship.", {.self = lone});
    EXPECT_NE(std::ranges::find(none, "self.Ship.speed"), none.end());  // the entity has one `Ship`: it settles it
    EXPECT_TRUE(game.host->complete("self.", {}).empty());
    const std::vector<std::string> prefixed = game.host->complete("ent", {});
    EXPECT_NE(std::ranges::find(prefixed, "entity"), prefixed.end());
}

TEST(ScriptConsoleEntities, WritesOnePartOfAVectorFieldInPlace) {
    const ScriptsDirectory scripts{"rtype-console-vector-part"};
    writeShipGame(scripts);
    Game game{scripts.path()};
    const auto ship = game.spawn("game.Ship");

    EXPECT_EQ(game.eval("self.Ship.position = { x = 1, y = 2 }", ship), "");
    EXPECT_EQ(game.eval("self.Ship.position.x = 5", ship), "");
    EXPECT_EQ(game.eval("self.Ship.position.x, self.Ship.position.y", ship), "5, 2");
    EXPECT_EQ(game.eval("self.Ship.position", ship), "{ x = 5, y = 2 }");
    EXPECT_TRUE(game.eval("self.Ship.position.z = 1", ship).find("no part 'z'") != std::string::npos);
    EXPECT_TRUE(game.eval("self.Ship.position.x = 1", ship, false).starts_with("error: "));
    EXPECT_EQ(game.eval("self.Ship.position.x", ship), "5");
    const auto parts = game.host->complete("self.Ship.position.", {.self = ship});
    EXPECT_NE(std::ranges::find(parts, "self.Ship.position.y"), parts.end());
}
