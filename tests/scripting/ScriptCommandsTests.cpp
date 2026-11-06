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
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
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

namespace {
/// @brief A C++ component with a list, as scripts see it.
struct Inventory {
    std::vector<std::uint32_t> items;
};
RTYPE_ECS_COMPONENT(Inventory, "test.script.Inventory", items);

/// @brief A C++ component with a text of any length.
struct Sign {
    std::string text;
};
RTYPE_ECS_COMPONENT(Sign, "test.script.Sign", text);
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

TEST(ScriptLists, ReadsAListAsATableOfItsValues) {
    Fixture fixture;
    rtype::ecs::World& world = fixture.app.getWorld();
    world.getComponents().add(world.spawn(), Inventory{.items = {4, 5, 6}});
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.ReadItems", {
            phase = "Update",
            query = { read = { "test.script.Inventory" } },
            run = function(ctx)
                for _, inventory in ctx.query do
                    items = inventory.items
                end
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(fixture.run("assert(#items == 3 and items[1] == 4 and items[3] == 6, tostring(#items))"), "");
}

TEST(ScriptLists, ReplacesAListFromATable) {
    Fixture fixture;
    rtype::ecs::World& world = fixture.app.getWorld();
    const rtype::ecs::Entity entity = world.spawn();
    world.getComponents().add(entity, Inventory{.items = {1, 2, 3}});
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.WriteItems", {
            phase = "Update",
            query = { write = { "test.script.Inventory" } },
            run = function(ctx)
                for _, inventory in ctx.query do
                    inventory.items = { 7, 8 }
                end
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(world.getComponents().get<Inventory>(entity)->items, (std::vector<std::uint32_t>{7, 8}));
}

TEST(ScriptLists, EmptiesAListFromAnEmptyTable) {
    Fixture fixture;
    rtype::ecs::World& world = fixture.app.getWorld();
    const rtype::ecs::Entity entity = world.spawn();
    world.getComponents().add(entity, Inventory{.items = {1}});
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.ClearItems", {
            phase = "Update",
            query = { write = { "test.script.Inventory" } },
            run = function(ctx)
                for _, inventory in ctx.query do
                    inventory.items = {}
                end
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_TRUE(world.getComponents().get<Inventory>(entity)->items.empty());
}

TEST(ScriptLists, RefusesBadValuesAndLeavesTheListAlone) {
    Fixture fixture;
    rtype::ecs::World& world = fixture.app.getWorld();
    const rtype::ecs::Entity entity = world.spawn();
    world.getComponents().add(entity, Inventory{.items = {1, 2}});
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.BadItems", {
            phase = "Update",
            query = { write = { "test.script.Inventory" } },
            run = function(ctx)
                for _, inventory in ctx.query do
                    notTable = pcall(function() inventory.items = 3 end)
                    wrongType = pcall(function() inventory.items = { "sword" } end)
                    outOfRange = pcall(function() inventory.items = { 1, -1 } end)
                    withHole = pcall(function() inventory.items = { [1] = 1, [3] = 3 } end)
                    named = pcall(function() inventory.items = { first = 1 } end)
                end
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(fixture.run("assert(not notTable and not wrongType and not outOfRange and not withHole and not named)"),
              "");
    EXPECT_EQ(world.getComponents().get<Inventory>(entity)->items, (std::vector<std::uint32_t>{1, 2}));
}

TEST(ScriptLists, ReadsAndWritesATextOfAnyLength) {
    Fixture fixture;
    rtype::ecs::World& world = fixture.app.getWorld();
    const rtype::ecs::Entity entity = world.spawn();
    world.getComponents().add(entity, Sign{.text = "hello"});
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Rewrite", {
            phase = "Update",
            query = { write = { "test.script.Sign" } },
            run = function(ctx)
                for _, sign in ctx.query do
                    seen = sign.text
                    sign.text = string.rep("a", 1000)
                    refused = pcall(function() sign.text = 3 end)
                end
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(fixture.run("assert(seen == 'hello' and refused == false)"), "");
    EXPECT_EQ(world.getComponents().get<Sign>(entity)->text, std::string(1000, 'a'));
}
