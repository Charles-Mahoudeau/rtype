/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptReloadTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ScriptsDirectory.hpp"
#include "rtype/core/reload/ReloadOutcome.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/ScriptHost.hpp"
#include "rtype/scripting/modules/ScriptResolver.hpp"

namespace {
using rtype::core::reload::ReloadOutcome;
using Status = rtype::core::reload::ReloadOutcome::Status;

/// @brief An App whose every frame runs one fixed tick of one second, and a host on it.
struct Fixture {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    Fixture() {
        app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}, rtype::ecs::FixedTimestep{1, 5}));
        auto created = rtype::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
        EXPECT_EQ(load("components.luau", R"(
            engine.ecs.component("game.Shield", { fields = { strength = "f32" } })
            engine.ecs.component("game.Position", { fields = { x = "f32" } })
        )"),
                  "");
    }

    /// @brief Runs a script for the first time and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string load(const std::string& file, const std::string& source) const {
        const auto result = host->run(file, source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @brief Gives a new entity the shield and the position, at zero.
    rtype::ecs::Entity spawn() {
        const rtype::ecs::Entity entity = app.getWorld().spawn();
        for (const std::string_view name : {"game.Shield", "game.Position"}) {
            const auto* layout = host->findComponent(name);
            EXPECT_TRUE(app.getWorld().getComponents().add(entity, layout->id, layout->defaults));
        }
        return entity;
    }

    /// @brief Reads the float field of an entity's component.
    [[nodiscard]] float read(const rtype::ecs::Entity entity, const std::string_view component,
                             const std::string_view name) const {
        const auto* layout = host->findComponent(component);
        const auto bytes = app.getWorld().getComponents().get(entity, layout->id);
        const auto field = std::ranges::find(layout->fields, name, &rtype::ecs::FieldInfo::name);
        float value = 0.F;
        std::memcpy(&value, bytes.data() + field->offset, sizeof(float));
        return value;
    }

    /// @return Whether a system of this name is in the fixed update schedule.
    [[nodiscard]] bool hasSystem(const std::string_view name) const {
        return app.getSchedules().getSchedule(rtype::ecs::Phase::kFixedUpdate).contains(name);
    }
};

/// @brief A host over a scripts directory, whose roots are the modules required by hand, as the entry points will be.
struct ModuleFixture {
    rtype::tests::ScriptsDirectory scripts;
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    explicit ModuleFixture(const std::string_view name) : scripts{name} {
        app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}, rtype::ecs::FixedTimestep{1, 5}));
        auto created = rtype::scripting::ScriptHost::create(
            app, {}, rtype::scripting::modules::ScriptResolver{scripts.path()}.forLoader());
        EXPECT_TRUE(created);
        host = std::move(*created);
    }

    /// @brief Requires a file as a root: @p key is `main` for `main.luau`.
    void require(const std::string& key) const {
        const auto result = host->require("./" + key);
        EXPECT_TRUE(result) << (result ? "" : std::string{result.error().message()});
    }

    /// @brief Writes the file and reloads it as a batch of one.
    [[nodiscard]] ReloadOutcome change(const std::string& key, const std::string& source) const {
        scripts.write(key + ".luau", source);
        return host->reloadModules({{"scripts/" + key + ".luau", source}}, {});
    }

    [[nodiscard]] bool hasSystem(const std::string_view name) const {
        return app.getSchedules().getSchedule(rtype::ecs::Phase::kFixedUpdate).contains(name);
    }
};

/// @return A script declaring the system @p name, which does nothing.
std::string noop(const std::string_view name) {
    return "engine.ecs.system('" + std::string{name} +
           "', { phase = 'FixedUpdate', query = {}, run = function(ctx) end })\n";
}

/// @return The position of @p what in @p list, or the size of the list.
std::size_t position(const std::vector<std::string>& list, const std::string_view what) {
    return static_cast<std::size_t>(std::ranges::find(list, what) - list.begin());
}

/// @return A script with the system `game.Grow` adding @p step to the shield each tick.
std::string grow(const int step) {
    return R"(
        engine.ecs.system("game.Grow", {
            phase = "FixedUpdate",
            query = { write = { "game.Shield" } },
            run = function(ctx)
                for entity, shield in ctx.query do shield.strength = shield.strength + )" +
           std::to_string(step) + R"( end
            end,
        })
    )";
}

constexpr std::string_view kMove = R"(
    engine.ecs.system("game.Move", {
        phase = "FixedUpdate",
        query = { write = { "game.Position" } },
        run = function(ctx)
            for entity, position in ctx.query do position.x = position.x + 1 end
        end,
    })
)";
}  // namespace

TEST(ScriptReload, ReplacesASystemAndKeepsTheWorld) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("grow.luau", grow(1)), "");
    const rtype::ecs::Entity ship = fixture.spawn();
    fixture.app.runFrame();
    ASSERT_FLOAT_EQ(fixture.read(ship, "game.Shield", "strength"), 1.F);

    const ReloadOutcome outcome = fixture.host->reloadScript("grow.luau", grow(10));

    EXPECT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_EQ(outcome.what, std::vector<std::string>{"system game.Grow"});
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(ship, "game.Shield", "strength"), 11.F) << "the entity and its value were kept";
}

TEST(ScriptReload, AddsTheSystemsOfANewScript) {
    Fixture fixture;
    const rtype::ecs::Entity ship = fixture.spawn();

    const ReloadOutcome outcome = fixture.host->reloadScript("move.luau", std::string{kMove});

    EXPECT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_TRUE(fixture.hasSystem("game.Move"));
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(ship, "game.Position", "x"), 1.F);
}

TEST(ScriptReload, RemovesTheSystemsAScriptNoLongerDeclares) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("both.luau", grow(1) + std::string{kMove}), "");
    const rtype::ecs::Entity ship = fixture.spawn();

    const ReloadOutcome outcome = fixture.host->reloadScript("both.luau", grow(1));

    EXPECT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_FALSE(fixture.hasSystem("game.Move"));
    EXPECT_TRUE(fixture.hasSystem("game.Grow"));
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(ship, "game.Position", "x"), 0.F);
    EXPECT_NE(std::ranges::find(outcome.what, "removed system game.Move"), outcome.what.end());
}

TEST(ScriptReload, RemovesTheSystemsOfADeletedScript) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("move.luau", std::string{kMove}), "");

    const ReloadOutcome outcome = fixture.host->removeScript("move.luau");

    EXPECT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_FALSE(fixture.hasSystem("game.Move"));
}

TEST(ScriptReload, KeepsTheOldSystemsWhenTheScriptDoesNotCompile) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("grow.luau", grow(1)), "");
    const rtype::ecs::Entity ship = fixture.spawn();

    const ReloadOutcome outcome = fixture.host->reloadScript("grow.luau", "this is not luau (");

    EXPECT_EQ(outcome.status, Status::kFailed);
    EXPECT_FALSE(outcome.message.empty());
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(ship, "game.Shield", "strength"), 1.F);
}

TEST(ScriptReload, ChangesNothingWhenTheScriptFailsAfterDeclaringASystem) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("grow.luau", grow(1)), "");
    const rtype::ecs::Entity ship = fixture.spawn();

    const ReloadOutcome outcome = fixture.host->reloadScript("grow.luau", grow(100) + "\nerror('boom')");

    EXPECT_EQ(outcome.status, Status::kFailed);
    EXPECT_NE(outcome.message.find("boom"), std::string::npos) << outcome.message;
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(ship, "game.Shield", "strength"), 1.F) << "still the old system";
}

TEST(ScriptReload, KeepsTheScheduleWhenTheNewOrderCannotBeBuilt) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("grow.luau", grow(1)), "");
    const rtype::ecs::Entity ship = fixture.spawn();
    const std::string orderedAfterNothing = R"(
        engine.ecs.system("game.Grow", {
            phase = "FixedUpdate",
            query = { write = { "game.Shield" } },
            after = { "game.Missing" },
            run = function(ctx) end,
        })
    )";

    const ReloadOutcome outcome = fixture.host->reloadScript("grow.luau", orderedAfterNothing);

    EXPECT_EQ(outcome.status, Status::kFailed);
    EXPECT_NE(outcome.message.find("game.Missing"), std::string::npos) << outcome.message;
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(ship, "game.Shield", "strength"), 1.F);
}

TEST(ScriptReload, TakesTheNewDefaultsOfAComponentWhoseLayoutDidNotChange) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("thing.luau", R"(engine.ecs.component("game.Thing", { fields = { a = "f32" } }))"), "");

    const ReloadOutcome outcome = fixture.host->reloadScript(
        "thing.luau", R"(engine.ecs.component("game.Thing", { fields = { a = "f32" }, defaults = { a = 5 } }))");

    EXPECT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_EQ(outcome.what, std::vector<std::string>{"component game.Thing"});
    const auto* layout = fixture.host->findComponent("game.Thing");
    ASSERT_NE(layout, nullptr);
    float value = 0.F;
    std::memcpy(&value, layout->defaults.data(), sizeof(float));
    EXPECT_FLOAT_EQ(value, 5.F);
}

TEST(ScriptReload, NeedsARebuildWhenAComponentChangesLayout) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("thing.luau", R"(engine.ecs.component("game.Thing", { fields = { a = "f32" } }))"), "");

    const ReloadOutcome outcome = fixture.host->reloadScript(
        "thing.luau", R"(engine.ecs.component("game.Thing", { fields = { a = "f32", b = "f32" } }))");

    EXPECT_EQ(outcome.status, Status::kNeedsRebuild);
    EXPECT_NE(outcome.message.find("game.Thing"), std::string::npos) << outcome.message;
}

TEST(ScriptReload, NeedsARebuildWhenAComponentDisappears) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("thing.luau", R"(engine.ecs.component("game.Thing", { fields = { a = "f32" } }))"), "");

    const ReloadOutcome outcome = fixture.host->reloadScript("thing.luau", "");

    EXPECT_EQ(outcome.status, Status::kNeedsRebuild);
    EXPECT_NE(outcome.message.find("game.Thing"), std::string::npos) << outcome.message;
}

TEST(ScriptReload, AddsANewComponentThatIsNotReplicated) {
    Fixture fixture;

    const ReloadOutcome outcome =
        fixture.host->reloadScript("thing.luau", R"(engine.ecs.component("game.Fresh", { fields = { a = "f32" } }))");

    EXPECT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_NE(fixture.host->findComponent("game.Fresh"), nullptr);
}

TEST(ScriptReload, NeedsARebuildForANewReplicatedComponent) {
    Fixture fixture;

    const ReloadOutcome outcome = fixture.host->reloadScript("thing.luau", R"(
        engine.ecs.component("game.Fresh", { fields = { a = "f32" }, replication = "everyChange" })
    )");

    EXPECT_EQ(outcome.status, Status::kNeedsRebuild);
    EXPECT_NE(outcome.message.find("game.Fresh"), std::string::npos) << outcome.message;
}

TEST(ScriptReload, NeedsARebuildWhenASystemChangesPhase) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("grow.luau", grow(1)), "");
    const std::string inUpdate = R"(
        engine.ecs.system("game.Grow", {
            phase = "Update",
            query = { write = { "game.Shield" } },
            run = function(ctx) end,
        })
    )";

    const ReloadOutcome outcome = fixture.host->reloadScript("grow.luau", inUpdate);

    EXPECT_EQ(outcome.status, Status::kNeedsRebuild);
    EXPECT_NE(outcome.message.find("game.Grow"), std::string::npos) << outcome.message;
}

TEST(ScriptReload, RefusesASystemAnotherScriptDeclared) {
    Fixture fixture;
    ASSERT_EQ(fixture.load("grow.luau", grow(1)), "");

    const ReloadOutcome outcome = fixture.host->reloadScript("other.luau", grow(2));

    EXPECT_EQ(outcome.status, Status::kFailed);
    EXPECT_NE(outcome.message.find("already declared by the script 'grow.luau'"), std::string::npos) << outcome.message;
}

TEST(ScriptReload, BuildsAgainALuauPluginTheScriptDeclaresAgain) {
    Fixture fixture;
    const auto plugin = [](const int step) {
        return R"(
            engine.ecs.plugin("game.Growth", {
                build = function(app)
                    app:system("game.Grow", {
                        phase = "FixedUpdate",
                        query = { write = { "game.Shield" } },
                        run = function(ctx)
                            for entity, shield in ctx.query do shield.strength = shield.strength + )" +
               std::to_string(step) + R"( end
                        end,
                    })
                end,
            })
            engine.ecs.addPlugin("game.Growth")
        )";
    };
    ASSERT_EQ(fixture.load("growth.luau", plugin(1)), "");
    const rtype::ecs::Entity ship = fixture.spawn();
    fixture.app.runFrame();
    ASSERT_FLOAT_EQ(fixture.read(ship, "game.Shield", "strength"), 1.F);

    const ReloadOutcome outcome = fixture.host->reloadScript("growth.luau", plugin(5));

    EXPECT_EQ(outcome.status, Status::kApplied) << outcome.message;
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(ship, "game.Shield", "strength"), 6.F);
}

TEST(ScriptReload, KeepsTheOldPluginWhenTheScriptFails) {
    Fixture fixture;
    const std::string source = R"(
        engine.ecs.plugin("game.Growth", { build = function(app) end })
        engine.ecs.addPlugin("game.Growth")
    )";
    ASSERT_EQ(fixture.load("growth.luau", source), "");

    const ReloadOutcome failed = fixture.host->reloadScript("growth.luau", source + "\nerror('boom')");
    const ReloadOutcome again = fixture.host->reloadScript("growth.luau", source);

    EXPECT_EQ(failed.status, Status::kFailed);
    EXPECT_EQ(again.status, Status::kApplied) << again.message;
}

TEST(ScriptReload, ReRunsTheDependentsOfAChangedModuleAfterIt) {
    ModuleFixture fixture{"rtype-reload-dependents"};
    fixture.scripts.write("lib.luau", noop("game.Lib") + "return { n = 1 }");
    fixture.scripts.write("mid.luau", "local lib = require('./lib')\n" + noop("game.Mid"));
    fixture.scripts.write("main.luau", "require('./mid')\n" + noop("game.Main"));
    fixture.scripts.write("other.luau", noop("game.Other"));
    fixture.require("main");
    fixture.require("other");

    const ReloadOutcome outcome = fixture.change("lib", noop("game.Lib") + "return { n = 2 }");

    ASSERT_EQ(outcome.status, Status::kApplied) << outcome.message;
    const auto& what = outcome.what;
    EXPECT_LT(position(what, "system game.Lib"), position(what, "system game.Mid"));
    EXPECT_LT(position(what, "system game.Mid"), position(what, "system game.Main"));
    EXPECT_EQ(position(what, "system game.Other"), what.size());
}

TEST(ScriptReload, TheDependentsSeeTheNewValue) {
    ModuleFixture fixture{"rtype-reload-value"};
    fixture.scripts.write("lib.luau", "return { n = 1 }");
    fixture.scripts.write("main.luau",
                          "engine.ecs.system('game.V' .. require('./lib').n, "
                          "{ phase = 'FixedUpdate', query = {}, run = function(ctx) end })");
    fixture.require("main");
    ASSERT_TRUE(fixture.hasSystem("game.V1"));

    ASSERT_EQ(fixture.change("lib", "return { n = 2 }").status, Status::kApplied);

    EXPECT_TRUE(fixture.hasSystem("game.V2"));
    EXPECT_FALSE(fixture.hasSystem("game.V1"));
}

TEST(ScriptReload, ABatchRunsAModuleChangedTwiceOnce) {
    ModuleFixture fixture{"rtype-reload-batch"};
    fixture.scripts.write("a.luau", "return 1");
    fixture.scripts.write("b.luau", "return 2");
    fixture.scripts.write("main.luau", "require('./a'); require('./b')\n" + noop("game.Main"));
    fixture.require("main");
    fixture.scripts.write("a.luau", "return 10");
    fixture.scripts.write("b.luau", "return 20");

    const ReloadOutcome outcome =
        fixture.host->reloadModules({{"scripts/a.luau", "return 10"}, {"scripts/b.luau", "return 20"}}, {});

    ASSERT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_EQ(std::ranges::count(outcome.what, "system game.Main"), 1);
}

TEST(ScriptReload, AFailingDependentRestoresTheSystemsAndTheCache) {
    ModuleFixture fixture{"rtype-reload-failure"};
    fixture.scripts.write("lib.luau", noop("game.Lib") + "return { n = 1 }");
    const std::string main = "assert(require('./lib').n == 1, 'boom')\n" + noop("game.Main");
    fixture.scripts.write("main.luau", main);
    fixture.require("main");

    // The new lib is only given in memory, so the disk still holds the old one.
    const ReloadOutcome outcome =
        fixture.host->reloadModules({{"scripts/lib.luau", noop("game.LibAgain") + "return { n = 2 }"}}, {});

    EXPECT_EQ(outcome.status, Status::kFailed);
    EXPECT_NE(outcome.message.find("boom"), std::string::npos) << outcome.message;
    EXPECT_TRUE(fixture.hasSystem("game.Lib"));
    EXPECT_TRUE(fixture.hasSystem("game.Main"));
    EXPECT_FALSE(fixture.hasSystem("game.LibAgain"));
    // The cache kept the old value: main, run again alone, still sees n == 1.
    EXPECT_EQ(fixture.host->reloadModules({{"scripts/main.luau", main}}, {}).status, Status::kApplied);
}

TEST(ScriptReload, ARemovedModuleThatIsStillRequiredFailsAndKeepsTheOldState) {
    ModuleFixture fixture{"rtype-reload-removed-required"};
    fixture.scripts.write("lib.luau", noop("game.Lib"));
    fixture.scripts.write("main.luau", "require('./lib')\n" + noop("game.Main"));
    fixture.require("main");
    std::filesystem::remove(fixture.scripts.path() / "lib.luau");

    const ReloadOutcome outcome = fixture.host->removeScript("scripts/lib.luau");

    EXPECT_EQ(outcome.status, Status::kFailed);
    EXPECT_NE(outcome.message.find("lib"), std::string::npos) << outcome.message;
    EXPECT_TRUE(fixture.hasSystem("game.Lib"));
    EXPECT_TRUE(fixture.hasSystem("game.Main"));
}

TEST(ScriptReload, ARemovedModuleNobodyRequiresLosesItsSystems) {
    ModuleFixture fixture{"rtype-reload-removed"};
    fixture.scripts.write("lib.luau", noop("game.Lib"));
    fixture.scripts.write("main.luau", noop("game.Main"));
    fixture.require("main");
    fixture.require("lib");
    std::filesystem::remove(fixture.scripts.path() / "lib.luau");

    const ReloadOutcome outcome = fixture.host->removeScript("scripts/lib.luau");

    EXPECT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_FALSE(fixture.hasSystem("game.Lib"));
    EXPECT_TRUE(fixture.hasSystem("game.Main"));
}

TEST(ScriptReload, AModuleNoLongerRequiredIsUnloadedWithItsSystems) {
    ModuleFixture fixture{"rtype-reload-unreachable"};
    fixture.scripts.write("lib.luau", noop("game.Lib"));
    fixture.scripts.write("main.luau", "require('./lib')\n" + noop("game.Main"));
    fixture.require("main");

    const ReloadOutcome outcome = fixture.change("main", noop("game.Main"));

    ASSERT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_FALSE(fixture.hasSystem("game.Lib"));
    EXPECT_TRUE(fixture.hasSystem("game.Main"));
}

TEST(ScriptReload, NeedsARebuildWhenTheModuleNoLongerRequiredDeclaredAComponent) {
    ModuleFixture fixture{"rtype-reload-unreachable-component"};
    fixture.scripts.write("lib.luau", "engine.ecs.component('game.C', { fields = {} })");
    fixture.scripts.write("main.luau", "require('./lib')");
    fixture.require("main");

    EXPECT_EQ(fixture.change("main", "").status, Status::kNeedsRebuild);
}

TEST(ScriptReload, ANewlyRequiredModuleIsLoaded) {
    ModuleFixture fixture{"rtype-reload-newly-required"};
    fixture.scripts.write("main.luau", noop("game.Main"));
    fixture.require("main");
    fixture.scripts.write("extra.luau", noop("game.Extra"));

    const ReloadOutcome outcome = fixture.change("main", "require('./extra')\n" + noop("game.Main"));

    ASSERT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_TRUE(fixture.hasSystem("game.Extra"));
}

TEST(ScriptReload, ANewFileIsARootUnlessTheHostIgnoresNewFiles) {
    ModuleFixture fixture{"rtype-reload-new-file"};
    fixture.scripts.write("main.luau", noop("game.Main"));
    fixture.require("main");
    ASSERT_EQ(fixture.change("added", noop("game.Added")).status, Status::kApplied);
    EXPECT_TRUE(fixture.hasSystem("game.Added"));

    fixture.host->setAdoptsNewFiles(false);
    const ReloadOutcome ignored = fixture.change("ignored", noop("game.Ignored"));

    EXPECT_EQ(ignored.status, Status::kApplied) << ignored.message;
    EXPECT_FALSE(fixture.hasSystem("game.Ignored"));
    // The root added earlier survives a reload of main.
    ASSERT_EQ(fixture.change("main", noop("game.Main")).status, Status::kApplied);
    EXPECT_TRUE(fixture.hasSystem("game.Added"));
}

TEST(ScriptReload, ASystemMovedBetweenTwoModulesInOneBatchIsAccepted) {
    ModuleFixture fixture{"rtype-reload-moved"};
    fixture.scripts.write("a.luau", noop("game.Moving"));
    fixture.scripts.write("b.luau", noop("game.Stays"));
    fixture.require("a");
    fixture.require("b");
    const std::string newA = noop("game.Stays2");
    const std::string newB = noop("game.Moving");
    fixture.scripts.write("a.luau", newA);
    fixture.scripts.write("b.luau", newB);

    const ReloadOutcome outcome = fixture.host->reloadModules({{"scripts/a.luau", newA}, {"scripts/b.luau", newB}}, {});

    ASSERT_EQ(outcome.status, Status::kApplied) << outcome.message;
    EXPECT_TRUE(fixture.hasSystem("game.Moving"));
    EXPECT_TRUE(fixture.hasSystem("game.Stays2"));
    EXPECT_FALSE(fixture.hasSystem("game.Stays"));
}

TEST(ScriptReload, ACycleIntroducedByAReloadFailsAndKeepsTheOldState) {
    ModuleFixture fixture{"rtype-reload-cycle"};
    fixture.scripts.write("a.luau", noop("game.A"));
    fixture.scripts.write("main.luau", "require('./a')\n" + noop("game.Main"));
    fixture.require("main");

    const ReloadOutcome outcome = fixture.change("a", "require('./main')\n" + noop("game.A2"));

    EXPECT_EQ(outcome.status, Status::kFailed);
    EXPECT_NE(outcome.message.find("cycle"), std::string::npos) << outcome.message;
    EXPECT_TRUE(fixture.hasSystem("game.A"));
    EXPECT_FALSE(fixture.hasSystem("game.A2"));
}
