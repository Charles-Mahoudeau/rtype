/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LuauPluginTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
/// @brief A bare App and a host on it.
struct Fixture {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    Fixture() {
        auto created = rtype::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("plugin-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    [[nodiscard]] bool hasSystem(const std::string_view name) const {
        return app.getSchedules().getSchedule(rtype::ecs::Phase::kUpdate).findSystem(name) != nullptr;
    }
};
}  // namespace

TEST(LuauPlugin, AddsAComponentAndASystemThroughBuild) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.plugin("game.Shields", {
            build = function(app)
                app:component("game.Shield", { fields = { strength = "f32" } })
                app:system("game.ShieldRegen", {
                    phase = "Update",
                    query = { write = { "game.Shield" } },
                    run = function(ctx) end,
                })
            end,
        })
        engine.ecs.addPlugin("game.Shields")
    )"),
              "");

    EXPECT_NE(fixture.host->findComponent("game.Shield"), nullptr);
    EXPECT_TRUE(fixture.hasSystem("game.ShieldRegen"));
}

TEST(LuauPlugin, IsNotBuiltUntilItIsAdded) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.plugin("game.Shields", {
            build = function(app) app:component("game.Shield", { fields = { strength = "f32" } }) end,
        })
    )"),
              "");
    EXPECT_EQ(fixture.host->findComponent("game.Shield"), nullptr);
}

TEST(LuauPlugin, AddsTheListedPluginsFirstAndOnce) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        order = {}
        engine.ecs.plugin("a", { build = function(app) table.insert(order, "a") end })
        engine.ecs.plugin("b", { plugins = { "a" }, build = function(app) table.insert(order, "b") end })
        engine.ecs.plugin("c", {
            plugins = { "a", "b" },
            build = function(app)
                table.insert(order, "c")
                app:addPlugin("a")
            end,
        })
        engine.ecs.addPlugin("c")
        engine.ecs.addPlugin("b")
        assert(table.concat(order, ",") == "a,b,c", table.concat(order, ","))
    )"),
              "");
}

TEST(LuauPlugin, AnAppFromAnotherPluginAddsThroughBuild) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.plugin("inner", {
            build = function(app) app:component("game.Inner", { fields = { x = "f32" } }) end,
        })
        engine.ecs.plugin("outer", { build = function(app) app:addPlugin("inner") end })
        engine.ecs.addPlugin("outer")
    )"),
              "");
    EXPECT_NE(fixture.host->findComponent("game.Inner"), nullptr);
}

TEST(LuauPlugin, ACycleIsRefusedWithTheNamesOfThePlugins) {
    Fixture fixture;
    const std::string error = fixture.run(R"(
        engine.ecs.plugin("a", { plugins = { "b" } })
        engine.ecs.plugin("b", { plugins = { "c" } })
        engine.ecs.plugin("c", { plugins = { "a" } })
        engine.ecs.addPlugin("a")
    )");
    EXPECT_NE(error.find("a -> b -> c -> a"), std::string::npos) << error;
}

TEST(LuauPlugin, ACycleThroughBuildIsRefusedWithTheNamesOfThePlugins) {
    Fixture fixture;
    const std::string error = fixture.run(R"(
        engine.ecs.plugin("a", { build = function(app) app:addPlugin("b") end })
        engine.ecs.plugin("b", { build = function(app) app:addPlugin("a") end })
        engine.ecs.addPlugin("a")
    )");
    EXPECT_NE(error.find("a -> b -> a"), std::string::npos) << error;
}

TEST(LuauPlugin, ASelfDependencyIsACycle) {
    Fixture fixture;
    const std::string error = fixture.run(R"(
        engine.ecs.plugin("a", { plugins = { "a" } })
        engine.ecs.addPlugin("a")
    )");
    EXPECT_NE(error.find("a -> a"), std::string::npos) << error;
}

TEST(LuauPlugin, ADuplicateNameIsRefused) {
    Fixture fixture;
    const std::string error = fixture.run(R"(
        engine.ecs.plugin("game.Shields", {})
        engine.ecs.plugin("game.Shields", {})
    )");
    EXPECT_NE(error.find("game.Shields"), std::string::npos) << error;
    EXPECT_NE(error.find("already declared"), std::string::npos) << error;
}

TEST(LuauPlugin, AnUnknownPluginIsRefusedWithItsName) {
    Fixture fixture;
    const std::string error = fixture.run(R"(
        engine.ecs.plugin("a", { plugins = { "missing" } })
        engine.ecs.addPlugin("a")
    )");
    EXPECT_NE(error.find("missing"), std::string::npos) << error;
}

TEST(LuauPlugin, ABadDeclarationIsRefusedWithTheNameOfThePlugin) {
    Fixture fixture;
    for (const char* spec : {"{ build = 1 }", "{ plugins = \"a\" }", "{ plugins = { 1 } }", "{ other = 1 }"}) {
        const std::string error = fixture.run(std::string{"engine.ecs.plugin(\"bad\", "} + spec + ")");
        EXPECT_NE(error.find("Plugin 'bad'"), std::string::npos) << spec << ": " << error;
    }
}

TEST(LuauPlugin, AFailingBuildIsReportedWithTheNameOfThePlugin) {
    Fixture fixture;
    const std::string error = fixture.run(R"(
        engine.ecs.plugin("boom", { build = function(app) error("nope") end })
        engine.ecs.addPlugin("boom")
    )");
    EXPECT_NE(error.find("Plugin 'boom'"), std::string::npos) << error;
    EXPECT_NE(error.find("nope"), std::string::npos) << error;
}

TEST(LuauPlugin, AnAppKeptAfterBuildCannotBeUsed) {
    Fixture fixture;
    const std::string error = fixture.run(R"(
        local kept
        engine.ecs.plugin("a", { build = function(app) kept = app end })
        engine.ecs.addPlugin("a")
        kept:component("game.Late", { fields = {} })
    )");
    EXPECT_NE(error.find("no longer valid"), std::string::npos) << error;
    EXPECT_EQ(fixture.host->findComponent("game.Late"), nullptr);
}
