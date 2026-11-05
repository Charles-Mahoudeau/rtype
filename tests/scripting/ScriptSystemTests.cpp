/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptSystemTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
/// @brief A bare App and a host on it, with the components the systems below use already declared.
struct Fixture {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    Fixture() {
        auto created = rtype::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
        EXPECT_EQ(run(R"(
            engine.ecs.component("game.Shield", { fields = { strength = "f32", regen = "f32" } })
            engine.ecs.component("game.Position", { fields = { x = "f32", y = "f32" } })
            engine.ecs.component("game.Dead", { fields = { } })
        )"),
                  "");
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("system-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @return The system of a phase with this name, or null.
    [[nodiscard]] const rtype::ecs::System* find(const rtype::ecs::Phase phase, const std::string_view name) const {
        return app.getSchedules().getSchedule(phase).findSystem(name);
    }

    [[nodiscard]] rtype::ecs::ComponentId id(const std::string_view name) {
        return *app.getWorld().getComponents().find(name);
    }
};
}  // namespace

TEST(ScriptSystem, IsListedUnderItsNameWithTheAccessOfItsTerms) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.system("game.ShieldRegen", {
            phase = "FixedUpdate",
            query = { write = { "game.Shield" }, read = { "game.Position" }, without = { "game.Dead" } },
            run = function(ctx) end,
        })
    )"),
              "");

    const auto* system = fixture.find(rtype::ecs::Phase::kFixedUpdate, "game.ShieldRegen");
    ASSERT_NE(system, nullptr);
    const auto access = system->getAccess().getComponents();
    ASSERT_EQ(access.size(), 2U);
    // Terms come out as read, write..., whatever the order of the keys.
    EXPECT_EQ(access[0].id, fixture.id("game.Position"));
    EXPECT_FALSE(access[0].write);
    EXPECT_EQ(access[1].id, fixture.id("game.Shield"));
    EXPECT_TRUE(access[1].write);
    EXPECT_EQ(fixture.find(rtype::ecs::Phase::kUpdate, "game.ShieldRegen"), nullptr);
}

TEST(ScriptSystem, AfterAndBeforeOrderTheSchedule) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        local run = function(ctx) end
        engine.ecs.system("game.C", { phase = "Update", query = { write = { "game.Shield" } }, run = run,
                                      after = { "game.B" } })
        engine.ecs.system("game.B", { phase = "Update", query = { write = { "game.Shield" } }, run = run })
        engine.ecs.system("game.A", { phase = "Update", query = { write = { "game.Shield" } }, run = run,
                                      before = { "game.B" } })
    )"),
              "");

    fixture.app.runFrame();  // builds the schedules
    const std::vector<std::string_view> expected{"game.A", "game.B", "game.C"};
    EXPECT_EQ(fixture.app.getSchedules().getSchedule(rtype::ecs::Phase::kUpdate).getOrder(), expected);
}

TEST(ScriptSystem, UnknownComponentIsRefusedWhenTheScriptLoads) {
    Fixture fixture;
    const auto error = fixture.run(R"(
        engine.ecs.system("game.Bad", { phase = "Update", query = { read = { "game.Nope" } }, run = function(ctx) end })
    )");
    EXPECT_NE(error.find("game.Bad"), std::string::npos) << error;
    EXPECT_NE(error.find("game.Nope"), std::string::npos) << error;
    EXPECT_EQ(fixture.find(rtype::ecs::Phase::kUpdate, "game.Bad"), nullptr);
}

TEST(ScriptSystem, BadDeclarationsRaiseAnErrorNamingTheSystem) {
    Fixture fixture;
    const std::vector<std::pair<std::string, std::string>> cases{
        {R"({ query = {}, run = function() end })", "needs a 'phase'"},
        {R"({ phase = "Update", run = function() end })", "needs a 'query'"},
        {R"({ phase = "Update", query = {} })", "needs a 'run'"},
        {R"({ phase = "Sometimes", query = {}, run = function() end })", "unknown phase 'Sometimes'"},
        {R"({ phase = "Update", query = {}, run = 3 })", "'run' must be a function"},
        {R"({ phase = "Update", query = {}, run = function() end, speed = 1 })", "unknown option 'speed'"},
        {R"({ phase = "Update", query = { maybe = {} }, run = function() end })", "unknown query term 'maybe'"},
        {R"({ phase = "Update", query = { added = { "game.Nope" } }, run = function() end })",
         "unknown component 'game.Nope'"},
        {R"({ phase = "Update", query = { changed = { "game.Nope" } }, run = function() end })",
         "unknown component 'game.Nope'"},
        {R"({ phase = "Update", query = { read = "game.Shield" }, run = function() end })", "must be a list"},
        {R"({ phase = "Update", query = { read = { 1 } }, run = function() end })", "must be a list of strings"},
        {R"({ phase = "Update", query = { read = { "game.Shield" }, write = { "game.Shield" } },
              run = function() end })",
         "invalid query"},
    };
    for (const auto& [spec, message] : cases) {
        const auto error = fixture.run("engine.ecs.system(\"game.Bad\", " + spec + ")");
        EXPECT_NE(error.find("System 'game.Bad'"), std::string::npos) << spec << " -> " << error;
        EXPECT_NE(error.find(message), std::string::npos) << spec << " -> " << error;
    }
}

TEST(ScriptSystem, DuplicateNameIsRefused) {
    Fixture fixture;
    const std::string declaration = R"(
        engine.ecs.system("game.Twice", { phase = "Update", query = { read = { "game.Shield" } }, run = function(ctx) end })
    )";
    EXPECT_EQ(fixture.run(declaration), "");
    EXPECT_NE(fixture.run(declaration).find("game.Twice"), std::string::npos);
}

TEST(ScriptSystem, StaysInTheAppWhenTheHostIsDestroyedFirst) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(
        engine.ecs.system("game.Orphan", { phase = "Update", query = { read = { "game.Shield" } },
                                           run = function(ctx) end })
    )"),
              "");

    fixture.host.reset();

    EXPECT_NE(fixture.find(rtype::ecs::Phase::kUpdate, "game.Orphan"), nullptr);
    // The App is destroyed after the host, with the system's function still referenced.
}
