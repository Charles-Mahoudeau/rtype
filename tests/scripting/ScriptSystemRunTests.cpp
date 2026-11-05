/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptSystemRunTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
/// @brief An App whose every frame lasts one second and runs one fixed tick of one second, and a host on it.
struct Fixture {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    Fixture() {
        app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}, rtype::ecs::FixedTimestep{1, 5}));
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
        const auto result = host->run("run-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }

    /// @brief Gives a new entity a component, its fields set to the given values.
    rtype::ecs::Entity spawn(rtype::ecs::Entity entity, const std::string_view component,
                             const std::vector<std::pair<std::string_view, float>>& values) {
        const auto* layout = host->findComponent(component);
        std::vector<std::byte> bytes = layout->defaults;
        for (const auto& [name, value] : values) {
            const auto field = std::ranges::find(layout->fields, name, &rtype::ecs::FieldInfo::name);
            std::memcpy(bytes.data() + field->offset, &value, sizeof(float));
        }
        EXPECT_TRUE(app.getWorld().getComponents().add(entity, layout->id, bytes));
        return entity;
    }

    /// @brief Reads a float field of an entity's component.
    [[nodiscard]] float read(const rtype::ecs::Entity entity, const std::string_view component,
                             const std::string_view name) const {
        const auto* layout = host->findComponent(component);
        const auto bytes = app.getWorld().getComponents().get(entity, layout->id);
        const auto field = std::ranges::find(layout->fields, name, &rtype::ecs::FieldInfo::name);
        float value = 0.F;
        std::memcpy(&value, bytes.data() + field->offset, sizeof(float));
        return value;
    }
};

constexpr std::string_view kShieldRegen = R"(
    engine.ecs.system("game.ShieldRegen", {
        phase = "FixedUpdate",
        query = { write = { "game.Shield" }, without = { "game.Dead" } },
        run = function(ctx)
            for entity, shield in ctx.query do
                shield.strength = math.min(shield.strength + shield.regen * ctx.fixedDelta, 100)
            end
        end,
    })
)";
}  // namespace

TEST(ScriptSystemRun, ShieldRegenRaisesStrengthOnEveryFixedTick) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(std::string{kShieldRegen}), "");
    const auto ship =
        fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {{"strength", 10.F}, {"regen", 2.F}});

    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(ship, "game.Shield", "strength"), 12.F);
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(ship, "game.Shield", "strength"), 14.F);
}

TEST(ScriptSystemRun, TheWithoutTermSkipsTheEntitiesThatHaveTheComponent) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(std::string{kShieldRegen}), "");
    const auto alive =
        fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {{"strength", 1.F}, {"regen", 1.F}});
    const auto dead = fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {{"strength", 1.F}, {"regen", 1.F}});
    ASSERT_TRUE(fixture.app.getWorld().getComponents().add(
        dead, *fixture.app.getWorld().getComponents().find("game.Dead"), {}));

    fixture.app.runFrame();

    EXPECT_FLOAT_EQ(fixture.read(alive, "game.Shield", "strength"), 2.F);
    EXPECT_FLOAT_EQ(fixture.read(dead, "game.Shield", "strength"), 1.F);
}

TEST(ScriptSystemRun, YieldsTheEntityAndOneViewPerDataTermInOrder) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Drift", {
            phase = "Update",
            query = { read = { "game.Position" }, write = { "game.Shield" }, optionalRead = { "game.Dead" } },
            run = function(ctx)
                for entity, position, shield, dead in ctx.query do
                    assert(typeof(entity) == "Entity")
                    assert(dead == nil)
                    shield.strength = position.x + ctx.delta
                end
            end,
        })
    )"),
              "");
    const auto entity = fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {});
    fixture.spawn(entity, "game.Position", {{"x", 4.F}});

    fixture.app.runFrame();

    EXPECT_FLOAT_EQ(fixture.read(entity, "game.Shield", "strength"), 5.F);
}

TEST(ScriptSystemRun, AReadTermGivesAViewThatRefusesWrites) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Cheat", {
            phase = "Update",
            query = { read = { "game.Shield" } },
            run = function(ctx)
                for entity, shield in ctx.query do
                    shield.strength = 100
                end
            end,
        })
    )"),
              "");
    const auto entity = fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {{"strength", 1.F}});

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    const auto log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("game.Cheat"), std::string::npos) << log;
    EXPECT_NE(log.find("read-only"), std::string::npos) << log;
    EXPECT_FLOAT_EQ(fixture.read(entity, "game.Shield", "strength"), 1.F);
}

TEST(ScriptSystemRun, AViewKeptAfterTheRunIsDead) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        local kept = nil
        engine.ecs.system("game.Keeper", {
            phase = "Update",
            query = { write = { "game.Shield" } },
            run = function(ctx)
                if kept then
                    kept.strength = 5
                end
                for entity, shield in ctx.query do
                    kept = shield
                end
            end,
        })
    )"),
              "");
    fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {});

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    const auto log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("no longer valid"), std::string::npos) << log;
}

TEST(ScriptSystemRun, AnErrorIsLoggedEveryTickAndTheOtherSystemsKeepRunning) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Broken", {
            phase = "Update",
            query = { write = { "game.Shield" } },
            run = function(ctx) error("boom") end,
        })
        engine.ecs.system("game.Healthy", {
            phase = "Update",
            query = { write = { "game.Shield" } },
            after = { "game.Broken" },
            run = function(ctx)
                for entity, shield in ctx.query do
                    shield.strength = shield.strength + 1
                end
            end,
        })
    )"),
              "");
    const auto entity = fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {});

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    fixture.app.runFrame();
    const auto log = testing::internal::GetCapturedStderr();

    const auto first = log.find("game.Broken");
    ASSERT_NE(first, std::string::npos) << log;
    EXPECT_NE(log.find("game.Broken", first + 1), std::string::npos) << log;  // once per tick
    EXPECT_NE(log.find("boom"), std::string::npos) << log;
    EXPECT_EQ(log.find("game.Healthy"), std::string::npos) << log;
    EXPECT_FLOAT_EQ(fixture.read(entity, "game.Shield", "strength"), 2.F);
}

TEST(ScriptSystemRun, ASystemOfADestroyedHostDoesNothing) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(std::string{kShieldRegen}), "");
    const auto ship =
        fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {{"strength", 10.F}, {"regen", 2.F}});
    const auto* layout = fixture.host->findComponent("game.Shield");
    const auto id = layout->id;
    fixture.host.reset();

    fixture.app.runFrame();

    float strength = 0.F;
    const auto bytes = fixture.app.getWorld().getComponents().get(ship, id);
    ASSERT_EQ(bytes.size(), 8U);
    std::memcpy(&strength, bytes.data() + sizeof(float), sizeof(float));  // regen sorts before strength
    EXPECT_FLOAT_EQ(strength, 10.F);
}

TEST(ScriptSystemRun, AnAddedTermRunsOnceForANewComponent) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Welcome", {
            phase = "Update",
            query = { added = { "game.Shield" }, write = { "game.Position" } },
            run = function(ctx)
                for entity, position in ctx.query do
                    position.x = position.x + 1
                end
            end,
        })
    )"),
              "");
    const auto entity = fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {});
    fixture.spawn(entity, "game.Position", {});

    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(entity, "game.Position", "x"), 1.F);
    fixture.app.runFrame();
    EXPECT_FLOAT_EQ(fixture.read(entity, "game.Position", "x"), 1.F);
}

TEST(ScriptSystemRun, AChangedTermRunsAfterAWriteAndNotOtherwise) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        local frame = 0
        engine.ecs.system("game.Poke", {
            phase = "Update",
            query = { write = { "game.Shield" } },
            before = { "game.Watch" },
            run = function(ctx)
                frame = frame + 1
                if frame ~= 2 then
                    return
                end
                for entity, shield in ctx.query do
                    shield.strength = shield.strength + 1
                end
            end,
        })
        engine.ecs.system("game.Watch", {
            phase = "Update",
            query = { changed = { "game.Shield" }, write = { "game.Position" } },
            run = function(ctx)
                for entity, position in ctx.query do
                    position.x = position.x + 1
                end
            end,
        })
    )"),
              "");
    const auto entity = fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {});
    fixture.spawn(entity, "game.Position", {});

    fixture.app.runFrame();  // the component was just added: it counts as changed, Poke wrote nothing
    EXPECT_FLOAT_EQ(fixture.read(entity, "game.Position", "x"), 1.F);
    fixture.app.runFrame();  // Poke writes the shield
    EXPECT_FLOAT_EQ(fixture.read(entity, "game.Position", "x"), 2.F);
    fixture.app.runFrame();  // nothing wrote it
    EXPECT_FLOAT_EQ(fixture.read(entity, "game.Position", "x"), 2.F);
}

TEST(ScriptSystemRun, ChangedAndAddedGiveNoViewOfTheirOwn) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Both", {
            phase = "Update",
            query = { read = { "game.Shield" }, changed = { "game.Shield" } },
            run = function(ctx)
                for entity, shield, extra in ctx.query do
                    assert(extra == nil)
                    assert(shield.strength == 3)
                end
            end,
        })
    )"),
              "");
    fixture.spawn(fixture.app.getWorld().spawn(), "game.Shield", {{"strength", 3.F}});

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    EXPECT_EQ(testing::internal::GetCapturedStderr(), "");
}
