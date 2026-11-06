/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptCommandsTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <string>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace {
/// @brief An App whose every frame lasts one second, and a host on it with a few components declared, and one marked
/// entity.
struct Fixture {
    rtype::ecs::App app;
    std::unique_ptr<rtype::scripting::ScriptHost> host;

    Fixture() {
        app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}, rtype::ecs::FixedTimestep{1, 5}));
        auto created = rtype::scripting::ScriptHost::create(app, {});
        EXPECT_TRUE(created) << (created ? "" : std::string{created.error().message()});
        host = std::move(*created);
        EXPECT_EQ(run(R"(
            engine.ecs.component("game.Shield", {
                fields = { strength = "f32", regen = "f32" },
                defaults = { strength = 50, regen = 2 },
            })
            engine.ecs.component("game.Dead", { fields = { } })
            engine.ecs.component("game.Marker", { fields = { } })
        )"),
                  "");
        // A system needs a required term to run: this one entity makes each system below run once per frame.
        auto& components = app.getWorld().getComponents();
        EXPECT_TRUE(components.add(app.getWorld().spawn(), *components.find("game.Marker"), {}));
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    [[nodiscard]] std::string run(const std::string& source) const {
        const auto result = host->run("commands-test", source);
        return result ? std::string{} : std::string{result.error().message()};
    }
};
}  // namespace

TEST(ScriptCommands, ASpawnerCreatesEntitiesASecondSystemSeesInTheNextPhase) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Spawner", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                local entity = ctx.commands:spawn({ ["game.Shield"] = { strength = 10 } })
                assert(typeof(entity) == "Entity")
                ctx.commands:add(entity, "game.Dead")
            end,
        })
        engine.ecs.system("game.Regen", {
            phase = "PostUpdate",
            query = { write = { "game.Shield" }, with = { "game.Dead" } },
            run = function(ctx)
                for entity, shield in ctx.query do
                    -- `strength` is the value given, `regen` the default
                    seenStrength = shield.strength + shield.regen
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.app.getWorld().getEntityCount(), 2U);
    EXPECT_EQ(fixture.run("assert(seenStrength == 12, tostring(seenStrength))"), "");
}

TEST(ScriptCommands, SpawningWithAnUnknownComponentIsAnErrorAndRecordsNothing) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Bad", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                ctx.commands:spawn({ ["game.Shield"] = { }, ["game.Nope"] = { } })
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("unknown component 'game.Nope'"), std::string::npos) << log;
    EXPECT_EQ(fixture.app.getWorld().getEntityCount(), 1U);
}

TEST(ScriptCommands, AFieldTheComponentDoesNotHaveIsAnError) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Bad", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                ctx.commands:spawn({ ["game.Shield"] = { mana = 1 } })
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("has no field 'mana'"), std::string::npos) << log;
    EXPECT_EQ(fixture.app.getWorld().getEntityCount(), 1U);
}

TEST(ScriptCommands, DespawningAStaleEntityDoesNothing) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        local ghost
        engine.ecs.system("game.Reaper", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                if ghost == nil then
                    ghost = ctx.commands:spawn({ ["game.Dead"] = { } })
                    ctx.commands:despawn(ghost)
                else
                    ctx.commands:despawn(ghost)
                    ctx.commands:add(ghost, "game.Shield")
                end
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    fixture.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_EQ(log, "");
    EXPECT_EQ(fixture.app.getWorld().getEntityCount(), 1U);
}

TEST(ScriptCommands, AHeldCommandsHandleIsRefusedOnceTheRunEnded) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        local kept
        engine.ecs.system("game.Keeper", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                if kept ~= nil then
                    kept:spawn()
                end
                kept = ctx.commands
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    fixture.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("no longer valid"), std::string::npos) << log;
}

TEST(ScriptCommands, SetParentAndRemoveParentLinkEntitiesAndDespawnCascades) {
    Fixture fixture;
    static_cast<void>(
        fixture.app.getWorld().getComponents().registerType<rtype::ecs::Parent>(rtype::ecs::kParentOptions));
    ASSERT_EQ(fixture.run(R"(
        local frame = 0
        local parent, first, second
        engine.ecs.system("game.Family", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                for _ in ctx.query do
                    frame += 1
                    if frame == 1 then
                        parent = ctx.commands:spawn()
                        first = ctx.commands:spawn()
                        second = ctx.commands:spawn()
                        ctx.commands:setParent(first, parent)
                        ctx.commands:setParent(second, parent)
                    elseif frame == 2 then
                        ctx.commands:removeParent(first)
                    elseif frame == 3 then
                        ctx.commands:despawn(parent)
                    end
                end
            end,
        })
        engine.ecs.system("game.Count", {
            phase = "PostUpdate",
            query = { read = { "rtype.Parent" } },
            run = function(ctx)
                linked = 0
                for _ in ctx.query do
                    linked += 1
                end
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(fixture.run("assert(linked == 2, tostring(linked))"), "");
    EXPECT_EQ(fixture.app.getWorld().getEntityCount(), 4U);
    fixture.app.runFrame();
    EXPECT_EQ(fixture.run("assert(linked == 1, tostring(linked))"), "");
    fixture.app.runFrame();
    EXPECT_EQ(fixture.app.getWorld().getEntityCount(), 2U);  // the marker and the detached child
}

TEST(ScriptCommands, SetParentRefusesWhatIsNotAnEntity) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Wrong", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                for _ in ctx.query do
                    wrong = pcall(function()
                        ctx.commands:setParent(ctx.commands:spawn(), ctx.commands)
                    end)
                end
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(fixture.run("assert(wrong == false, tostring(wrong))"), "");
}
