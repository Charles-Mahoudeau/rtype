/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptCommandsTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/ScriptHost.hpp"

namespace rtype::test::commands {
/// @brief A plain-data C++ component, with defaults of its own.
struct Hull {
    float armor{7.0F};
    std::uint32_t plates{3};
};
RTYPE_ECS_COMPONENT(Hull, "test.Hull", armor, plates);

/// @brief A replicated C++ component.
struct Beacon {
    std::uint32_t channel{0};
};
RTYPE_ECS_COMPONENT(Beacon, "test.Beacon", channel);

/// @brief A C++ component that owns memory: a list and a text.
struct Cargo {
    std::vector<std::int32_t> items;
    std::string label{"empty"};
};
RTYPE_ECS_COMPONENT(Cargo, "test.Cargo", items, label);

/// @brief A value with no default: it must be given one.
struct Sealed {
    explicit Sealed(std::string seal) : seal{std::move(seal)} {}
    ~Sealed() = default;
    Sealed(const Sealed& other) = default;
    Sealed& operator=(const Sealed& other) = default;
    Sealed(Sealed&& other) noexcept = default;
    Sealed& operator=(Sealed&& other) noexcept = default;

    std::string seal;
};

/// @brief A C++ component that owns memory and has no default constructor.
struct Crate {
    Sealed sealed;
};
RTYPE_ECS_COMPONENT(Crate, "test.Crate");
}  // namespace rtype::test::commands

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

TEST(ScriptHierarchy, ScriptsReadChildrenAndParentThroughDeclaredTerms) {
    Fixture fixture;
    rtype::ecs::World& world = fixture.app.getWorld();
    static_cast<void>(world.getComponents().registerType<rtype::ecs::Parent>(rtype::ecs::kParentOptions));
    static_cast<void>(world.getComponents().registerType<rtype::ecs::Children>());
    const rtype::ecs::Entity parent = world.spawn();
    const rtype::ecs::Entity first = world.spawn();
    const rtype::ecs::Entity second = world.spawn();
    EXPECT_TRUE(world.setParent(first, parent));
    EXPECT_TRUE(world.setParent(second, parent));
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.ReadChildren", {
            phase = "Update",
            query = { read = { "rtype.Children" } },
            run = function(ctx)
                for entity, children in ctx.query do
                    owner = entity
                    list = children.entities
                end
            end,
        })
        engine.ecs.system("game.ReadParent", {
            phase = "Update",
            query = { read = { "rtype.Parent" } },
            run = function(ctx)
                parents = {}
                for entity, parent in ctx.query do
                    parents[entity.bits] = parent.entity
                end
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(fixture.run("assert(#list == 2, tostring(#list))"), "");
    EXPECT_EQ(fixture.run("assert(typeof(list[1]) == 'Entity')"), "");
    EXPECT_EQ(fixture.run("assert(parents[list[1].bits] == owner and parents[list[2].bits] == owner)"), "");
    EXPECT_EQ(fixture.run("assert(list[1] ~= list[2])"), "");
}

TEST(ScriptHierarchy, ChangingTheListGivenToAScriptChangesNothing) {
    Fixture fixture;
    rtype::ecs::World& world = fixture.app.getWorld();
    static_cast<void>(world.getComponents().registerType<rtype::ecs::Children>());
    const rtype::ecs::Entity parent = world.spawn();
    const rtype::ecs::Entity child = world.spawn();
    EXPECT_TRUE(world.setParent(child, parent));
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Tamper", {
            phase = "Update",
            query = { read = { "rtype.Children" } },
            run = function(ctx)
                for _, children in ctx.query do
                    local list = children.entities
                    list[1] = nil
                end
            end,
        })
    )"),
              "");
    fixture.app.runFrame();
    EXPECT_EQ(world.getComponents().get<rtype::ecs::Children>(parent)->entities.size(), 1U);
}

TEST(ScriptHierarchy, RefusesASystemWritingTheHierarchy) {
    Fixture fixture;
    auto& components = fixture.app.getWorld().getComponents();
    static_cast<void>(components.registerType<rtype::ecs::Parent>(rtype::ecs::kParentOptions));
    static_cast<void>(components.registerType<rtype::ecs::Children>());
    const std::string parent = fixture.run(R"(
        engine.ecs.system("game.WriteParent", {
            phase = "Update",
            query = { write = { "rtype.Parent" } },
            run = function(ctx) end,
        })
    )");
    EXPECT_NE(parent.find("cannot be written"), std::string::npos) << parent;
    const std::string children = fixture.run(R"(
        engine.ecs.system("game.WriteChildren", {
            phase = "Update",
            query = { optionalWrite = { "rtype.Children" }, with = { "game.Marker" } },
            run = function(ctx) end,
        })
    )");
    EXPECT_NE(children.find("cannot be written"), std::string::npos) << children;
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

TEST(ScriptCommands, AScriptSpawnsACxxComponentFromItsCxxDefaults) {
    Fixture fixture;
    static_cast<void>(fixture.app.getWorld().getComponents().registerType<rtype::test::commands::Hull>());
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Builder", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                ctx.commands:spawn({ ["test.Hull"] = { plates = 5 } })
            end,
        })
        engine.ecs.system("game.Inspector", {
            phase = "PostUpdate",
            query = { read = { "test.Hull" } },
            run = function(ctx)
                for entity, hull in ctx.query do
                    seenArmor, seenPlates = hull.armor, hull.plates
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.run("assert(seenArmor == 7 and seenPlates == 5, tostring(seenArmor))"), "");
}

TEST(ScriptCommands, AClientSystemCannotAddAReplicatedCxxComponent) {
    Fixture fixture;
    static_cast<void>(fixture.app.getWorld().getComponents().registerType<rtype::test::commands::Beacon>(
        {.replication = rtype::ecs::ReplicationMode::kEveryChange}));
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Lighter", {
            phase = "Update",
            side = "client",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                ctx.commands:spawn({ ["test.Beacon"] = { channel = 2 } })
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("a client system cannot add 'test.Beacon'"), std::string::npos) << log;
    EXPECT_EQ(fixture.app.getWorld().getEntityCount(), 1U);
}

TEST(ScriptCommands, AScriptSpawnsACxxComponentThatOwnsMemory) {
    Fixture fixture;
    static_cast<void>(fixture.app.getWorld().getComponents().registerType<rtype::test::commands::Cargo>());
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Loader", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                -- Longer than a std::string keeps inline: the copy must own its own buffer.
                ctx.commands:spawn({ ["test.Cargo"] = { items = { 4, 5, 6 }, label = string.rep("cargo ", 8) } })
                ctx.commands:add(ctx.commands:spawn(), "test.Cargo")
            end,
        })
        engine.ecs.system("game.Inspector", {
            phase = "PostUpdate",
            query = { read = { "test.Cargo" } },
            run = function(ctx)
                seen = {}
                for _, cargo in ctx.query do
                    seen[cargo.label] = #cargo.items
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.run(R"(assert(seen[string.rep("cargo ", 8)] == 3 and seen["empty"] == 0, "wrong cargo"))"), "");
}

TEST(ScriptCommands, ACxxComponentWithNoDefaultConstructorIsRefused) {
    Fixture fixture;
    static_cast<void>(fixture.app.getWorld().getComponents().registerType<rtype::test::commands::Crate>());
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Packer", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                ctx.commands:spawn({ ["test.Crate"] = { } })
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("cannot add 'test.Crate': it has no default constructor to start from"), std::string::npos)
        << log;
    EXPECT_EQ(fixture.app.getWorld().getEntityCount(), 1U);
}

TEST(ScriptCommands, PatchWritesOnlyTheFieldsItNames) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        local spawned = false
        engine.ecs.system("game.Spawner", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                if spawned then return end
                spawned = true
                local entity = ctx.commands:spawn({ ["game.Shield"] = { strength = 10, regen = 4 } })
                ctx.commands:patch(entity, "game.Shield", { strength = 20 })   -- after the spawn: regen stays
                ctx.commands:patch(ctx.commands:spawn(), "game.Shield", { strength = 1 })   -- no shield: skipped
            end,
        })
        engine.ecs.system("game.Inspector", {
            phase = "PostUpdate",
            query = { read = { "game.Shield" } },
            run = function(ctx)
                shields = 0
                for _, shield in ctx.query do
                    shields += 1
                    seenStrength, seenRegen = shield.strength, shield.regen
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.run("assert(shields == 1, tostring(shields))"), "");
    EXPECT_EQ(fixture.run("assert(seenStrength == 20 and seenRegen == 4, tostring(seenRegen))"), "");
}

TEST(ScriptCommands, PatchWritesTheFieldsOfACxxComponentThatOwnsMemory) {
    Fixture fixture;
    static_cast<void>(fixture.app.getWorld().getComponents().registerType<rtype::test::commands::Cargo>());
    ASSERT_EQ(fixture.run(R"(
        local spawned = false
        engine.ecs.system("game.Loader", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                if spawned then return end
                spawned = true
                local first = ctx.commands:spawn({ ["test.Cargo"] = { items = { 1, 2 }, label = "first" } })
                ctx.commands:patch(first, "test.Cargo", { label = string.rep("patched ", 8) })
                local second = ctx.commands:spawn({ ["test.Cargo"] = { items = { 1, 2 }, label = "second" } })
                ctx.commands:patch(second, "test.Cargo", { items = { 9, 9, 9 } })
            end,
        })
        engine.ecs.system("game.Inspector", {
            phase = "PostUpdate",
            query = { read = { "test.Cargo" } },
            run = function(ctx)
                seen = {}
                for _, cargo in ctx.query do
                    seen[cargo.label] = #cargo.items
                end
            end,
        })
    )"),
              "");

    fixture.app.runFrame();

    EXPECT_EQ(fixture.run(R"(assert(seen[string.rep("patched ", 8)] == 2 and seen["second"] == 3, "wrong cargo"))"),
              "");
}

TEST(ScriptCommands, PatchRefusesAFieldTheComponentDoesNotHaveAndRecordsNothing) {
    Fixture fixture;
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Typo", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                local entity = ctx.commands:spawn({ ["game.Shield"] = { strength = 10 } })
                ctx.commands:patch(entity, "game.Shield", { strenght = 20 })
            end,
        })
        engine.ecs.system("game.Inspector", {
            phase = "PostUpdate",
            query = { read = { "game.Shield" } },
            run = function(ctx)
                for _, shield in ctx.query do
                    seenStrength = shield.strength
                end
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("strenght"), std::string::npos) << log;
    EXPECT_EQ(fixture.run("assert(seenStrength == 10, tostring(seenStrength))"), "")
        << "the spawn before the error is applied, the patch is not";
}

TEST(ScriptCommands, AClientSystemCannotPatchAReplicatedCxxComponent) {
    Fixture fixture;
    static_cast<void>(fixture.app.getWorld().getComponents().registerType<rtype::test::commands::Beacon>(
        {.replication = rtype::ecs::ReplicationMode::kEveryChange}));
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Tuner", {
            phase = "Update",
            side = "client",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                ctx.commands:patch(ctx.commands:spawn(), "test.Beacon", { channel = 2 })
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("a client system cannot add 'test.Beacon'"), std::string::npos) << log;
}

TEST(ScriptCommands, PatchRefusesTheHierarchy) {
    Fixture fixture;
    static_cast<void>(
        fixture.app.getWorld().getComponents().registerType<rtype::ecs::Parent>(rtype::ecs::kParentOptions));
    ASSERT_EQ(fixture.run(R"(
        engine.ecs.system("game.Reparent", {
            phase = "Update",
            query = { with = { "game.Marker" } },
            run = function(ctx)
                local parent = ctx.commands:spawn()
                ctx.commands:patch(ctx.commands:spawn(), "rtype.Parent", { entity = parent })
            end,
        })
    )"),
              "");

    testing::internal::CaptureStderr();
    fixture.app.runFrame();
    const std::string log = testing::internal::GetCapturedStderr();

    EXPECT_NE(log.find("cannot patch 'rtype.Parent': use setParent and removeParent"), std::string::npos) << log;
}
