/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptSystemTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
/// @brief A bare App and a host on it, with the components the systems below use already declared.
struct Fixture {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    explicit Fixture(const rtype::ecs::RunMode mode = rtype::ecs::RunMode::kStandalone) : app{mode} {
        auto created = rtype::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
        EXPECT_EQ(run(R"(
            engine.ecs.component("game.Shield", { fields = { strength = "f32", regen = "f32" } })
            engine.ecs.component("game.Position", { fields = { x = "f32", y = "f32" } })
            engine.ecs.component("game.Dead", { fields = { } })
            engine.ecs.component("game.Synced", { fields = { value = "f32" }, replication = "everyChange" })
            engine.ecs.component("game.Glow", { fields = { intensity = "f32" }, presence = "clientOnly" })
            engine.ecs.component("game.Secret", { fields = { code = "f32" }, presence = "serverOnly" })
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

namespace {
/// @brief Declares a system of the Update phase, with the given side option, that writes a component.
[[nodiscard]] std::string writer(const std::string& side, const std::string& component) {
    return R"(engine.ecs.system("game.Writer", { phase = "Update", )" + side + R"( query = { write = { ")" + component +
           R"(" } }, run = function(ctx) end }))";
}

/// @brief Runs one frame of an App that has a "game.Glow" on an entity, and returns the intensity afterwards.
[[nodiscard]] float intensityAfterAFrame(const rtype::ecs::RunMode mode, const std::string& side) {
    Fixture fixture{mode};
    EXPECT_EQ(fixture.run(R"(engine.ecs.system("game.Writer", {
        phase = "Update", )" +
                          side + R"(
        query = { write = { "game.Glow" } },
        run = function(ctx)
            for entity, glow in ctx.query do
                glow.intensity = 1
            end
        end,
    }))"),
              "");
    auto& components = fixture.app.getWorld().getComponents();
    const auto* layout = fixture.host->findComponent("game.Glow");
    const auto entity = fixture.app.getWorld().spawn();
    EXPECT_TRUE(components.add(entity, layout->id, layout->defaults));

    fixture.app.runFrame();

    float intensity = 0.F;
    std::memcpy(&intensity, components.get(entity, layout->id).data(), sizeof(float));
    return intensity;
}
}  // namespace

TEST(ScriptSystem, RunsOnTheServerWhenItDeclaresNoSide) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(writer("", "game.Shield")), "");
    EXPECT_EQ(fixture.find(rtype::ecs::Phase::kUpdate, "game.Writer")->getSide(), rtype::ecs::Side::kServer);
}

TEST(ScriptSystem, SideIsSetOnTheSystem) {
    for (const auto& [name, side] :
         std::vector<std::pair<std::string, rtype::ecs::Side>>{{"server", rtype::ecs::Side::kServer},
                                                               {"client", rtype::ecs::Side::kClient},
                                                               {"both", rtype::ecs::Side::kBoth}}) {
        Fixture fixture;
        EXPECT_EQ(fixture.run(writer("side = \"" + name + "\",", "game.Glow")), "") << name;
        EXPECT_EQ(fixture.find(rtype::ecs::Phase::kUpdate, "game.Writer")->getSide(), side) << name;
    }
}

TEST(ScriptSystem, UnknownSideIsRefused) {
    Fixture fixture;
    const auto error = fixture.run(writer("side = \"everywhere\",", "game.Glow"));
    EXPECT_NE(error.find("System 'game.Writer'"), std::string::npos) << error;
    EXPECT_NE(error.find("'side'"), std::string::npos) << error;
    EXPECT_NE(fixture.run(writer("side = 3,", "game.Glow")).find("'side'"), std::string::npos);
}

TEST(ScriptSystem, AClientSystemDoesNotRunInAServerApp) {
    EXPECT_FLOAT_EQ(intensityAfterAFrame(rtype::ecs::RunMode::kServer, "side = \"client\","), 0.F);
}

TEST(ScriptSystem, AClientSystemRunsInAClientApp) {
    EXPECT_FLOAT_EQ(intensityAfterAFrame(rtype::ecs::RunMode::kClient, "side = \"client\","), 1.F);
}

TEST(ScriptSystem, AServerSystemDoesNotRunInAClientApp) {
    EXPECT_FLOAT_EQ(intensityAfterAFrame(rtype::ecs::RunMode::kClient, ""), 0.F);
    EXPECT_FLOAT_EQ(intensityAfterAFrame(rtype::ecs::RunMode::kServer, ""), 1.F);
}

TEST(ScriptSystem, AClientSystemWritingAReplicatedComponentFailsToLoad) {
    Fixture fixture;
    const auto error = fixture.run(writer("side = \"client\",", "game.Synced"));
    EXPECT_NE(error.find("game.Writer"), std::string::npos) << error;
    EXPECT_NE(error.find("game.Synced"), std::string::npos) << error;
}

TEST(ScriptSystem, AClientSystemMayReadAReplicatedComponent) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(R"(engine.ecs.system("game.Reader", {
        phase = "Update", side = "client",
        query = { read = { "game.Synced" }, write = { "game.Glow" } },
        run = function(ctx) end,
    }))"),
              "");
}

TEST(ScriptSystem, AClientSystemMayWriteAComponentNothingReplicates) {
    Fixture fixture;
    EXPECT_EQ(fixture.run(writer("side = \"client\",", "game.Shield")), "");
}

TEST(ScriptSystem, ASystemOfBothSidesMayWriteAReplicatedComponent) {
    Fixture fixture;
    testing::internal::CaptureStderr();
    EXPECT_EQ(fixture.run(writer("side = \"both\",", "game.Synced")), "");
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
}

TEST(ScriptSystem, ASystemOfBothSidesWritingAServerOnlyComponentWarns) {
    Fixture fixture;
    testing::internal::CaptureStderr();
    EXPECT_EQ(fixture.run(writer("side = \"both\",", "game.Secret")), "");
    const std::string warning = testing::internal::GetCapturedStderr();
    EXPECT_NE(warning.find("game.Writer"), std::string::npos) << warning;
    EXPECT_NE(warning.find("game.Secret"), std::string::npos) << warning;
    EXPECT_NE(fixture.find(rtype::ecs::Phase::kUpdate, "game.Writer"), nullptr);
}
