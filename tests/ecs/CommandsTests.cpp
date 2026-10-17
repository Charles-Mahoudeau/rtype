/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandsTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <utility>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
/// @brief Number of Tracked values alive, to check that every value is destroyed exactly once.
int liveTracked = 0;
}  // namespace

namespace rtype::test::commands {
/// @brief Counts its own instances in liveTracked.
struct Counter {
    Counter() noexcept { ++liveTracked; }
    ~Counter() { --liveTracked; }
    Counter(const Counter& /*other*/) noexcept { ++liveTracked; }
    Counter& operator=(const Counter& /*other*/) noexcept = default;
    Counter(Counter&& /*other*/) noexcept { ++liveTracked; }
    Counter& operator=(Counter&& /*other*/) noexcept = default;
};

struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "test.commands.Position", x, y);

struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.commands.Health", value);

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "test.commands.Dead");

/// @brief Owns memory, and counts its instances through its Counter.
struct Tracked {
    std::string name;
    Counter counter;
};
RTYPE_ECS_COMPONENT(Tracked, "test.commands.Tracked");
}  // namespace rtype::test::commands

namespace {
using rtype::ecs::Commands;
using rtype::ecs::ComponentId;
using rtype::ecs::Entity;
using rtype::ecs::World;
using rtype::test::commands::Dead;
using rtype::test::commands::Health;
using rtype::test::commands::Position;
using rtype::test::commands::Tracked;

/// @return The bytes of a value, as the Luau bridge passes them.
template <typename T>
std::span<const std::byte> bytesOf(const T& value) {
    return std::as_bytes(std::span{&value, 1});
}
}  // namespace

TEST(Commands, StartsEmpty) {
    World world;
    const Commands commands{world};
    EXPECT_TRUE(commands.isEmpty());
    EXPECT_EQ(commands.getCount(), 0U);
}

TEST(Commands, ASpawnedEntityIsUsableBeforeTheFlushAndAliveAfter) {
    World world;
    Commands commands{world};
    const Entity ship = commands.spawn().getEntity();
    EXPECT_FALSE(world.isAlive(ship));
    commands.add(ship, Position{.x = 1.F, .y = 2.F});  // usable at once
    EXPECT_FALSE(world.getComponents().has<Position>(ship));

    commands.flush();
    EXPECT_TRUE(world.isAlive(ship));
    ASSERT_NE(world.getComponents().get<Position>(ship), nullptr);
    EXPECT_EQ(world.getComponents().get<Position>(ship)->y, 2.F);
    EXPECT_TRUE(commands.isEmpty());
}

TEST(Commands, AppliesNothingBeforeTheFlush) {
    World world;
    const Entity enemy = world.spawn(Health{.value = 3});
    Commands commands{world};
    commands.remove<Health>(enemy);
    commands.add(enemy, Dead{});
    commands.despawn(enemy);
    EXPECT_EQ(commands.getCount(), 3U);
    EXPECT_TRUE(world.isAlive(enemy));
    EXPECT_TRUE(world.getComponents().has<Health>(enemy));
    EXPECT_FALSE(world.getComponents().has<Dead>(enemy));
}

TEST(Commands, AddThenRemoveLeavesNoComponent) {
    World world;
    world.getComponents().registerType<Health>();
    const Entity registered = world.spawn();
    const Entity fresh = world.spawn();
    Commands commands{world};
    commands.add(registered, Health{.value = 1});
    commands.remove<Health>(registered);
    commands.add(fresh, Position{});  // Position is not registered yet
    commands.remove<Position>(fresh);
    commands.flush();
    EXPECT_FALSE(world.getComponents().has<Health>(registered));
    EXPECT_FALSE(world.getComponents().has<Position>(fresh));
}

TEST(Commands, AppliesInRecordingOrder) {
    World world;
    const Entity entity = world.spawn();
    Commands commands{world};
    commands.add(entity, Health{.value = 1});
    commands.add(entity, Health{.value = 2});  // replaces the first
    commands.remove<Health>(entity);
    commands.add(entity, Health{.value = 3});
    commands.flush();
    ASSERT_NE(world.getComponents().get<Health>(entity), nullptr);
    EXPECT_EQ(world.getComponents().get<Health>(entity)->value, 3);
}

TEST(Commands, SkipsCommandsOnDeadEntities) {
    World world;
    const Entity enemy = world.spawn();
    const Entity gone = world.spawn();
    static_cast<void>(world.despawn(gone));
    Commands commands{world};
    commands.despawn(enemy);
    commands.add(enemy, Health{.value = 1});  // despawned by the command before: skipped
    commands.add(gone, Health{.value = 2});   // already dead when recorded: skipped
    commands.remove<Health>(gone);
    commands.despawn(gone);
    EXPECT_NO_THROW(commands.flush());
    EXPECT_FALSE(world.isAlive(enemy));
    EXPECT_EQ(world.getEntityCount(), 0U);
}

TEST(Commands, RegistersComponentsAtTheFlushOnly) {
    World world;
    const Entity entity = world.spawn();
    Commands commands{world};
    commands.add(entity, Position{});
    commands.remove<Health>(entity);
    EXPECT_EQ(world.getComponents().getRegistry().find<Position>(), nullptr);  // recording never registers
    EXPECT_EQ(world.getComponents().getRegistry().find<Health>(), nullptr);
    commands.flush();
    EXPECT_NE(world.getComponents().getRegistry().find<Position>(), nullptr);
    EXPECT_TRUE(world.getComponents().has<Position>(entity));
}

TEST(Commands, KeepsTheOptionsOfAnAlreadyRegisteredComponent) {
    World world;
    world.getComponents().registerType<Position>(
        rtype::ecs::ComponentOptions{.replication = rtype::ecs::ReplicationMode::kEveryChange});
    const Entity entity = world.spawn();
    Commands commands{world};
    commands.add(entity, Position{});
    EXPECT_NO_THROW(commands.flush());
    EXPECT_TRUE(world.getComponents().has<Position>(entity));
}

TEST(Commands, AddsAndRemovesById) {
    World world;
    const ComponentId health = world.getComponents().registerType<Health>();
    const Entity entity = world.spawn();
    Commands commands{world};
    const Health value{.value = 9};
    EXPECT_TRUE(commands.add(entity, health, bytesOf(value)));
    commands.flush();
    EXPECT_EQ(world.getComponents().get<Health>(entity)->value, 9);  // a copy: `value` is untouched
    EXPECT_TRUE(commands.remove(entity, health));
    commands.flush();
    EXPECT_FALSE(world.getComponents().has<Health>(entity));
}

TEST(Commands, RefusesInvalidCommandsById) {
    World world;
    const ComponentId health = world.getComponents().registerType<Health>();
    const Entity entity = world.spawn();
    Commands commands{world};
    EXPECT_FALSE(commands.add(entity, health + 1, bytesOf(Health{})));     // unknown id
    EXPECT_FALSE(commands.add(entity, health, bytesOf(std::int64_t{0})));  // 8 bytes, the component has 4
    EXPECT_FALSE(commands.remove(entity, health + 1));
    EXPECT_TRUE(commands.isEmpty());
}

TEST(Commands, ShareOneColumnBetweenACppTypeAndItsId) {
    World world;
    const ComponentId health = world.getComponents().registerType<Health>();
    const Entity entity = world.spawn();
    Commands commands{world};
    commands.add(entity, Health{.value = 1});
    EXPECT_TRUE(commands.add(entity, health, bytesOf(Health{.value = 2})));
    commands.flush();
    EXPECT_EQ(world.getComponents().get<Health>(entity)->value, 2);  // one component, applied in order
}

TEST(Commands, DespawnsAndCancelsSpawns) {
    World world;
    const Entity enemy = world.spawn();
    Commands commands{world};
    const Entity bullet = commands.spawn().getEntity();
    commands.despawn(bullet);  // spawned then despawned in the same buffer
    commands.despawn(enemy);
    commands.flush();
    EXPECT_FALSE(world.isAlive(bullet));
    EXPECT_FALSE(world.isAlive(enemy));
    EXPECT_EQ(world.getEntityCount(), 0U);
}

TEST(Commands, MovesValuesAndDestroysEachOnce) {
    liveTracked = 0;
    {
        World world;
        const Entity entity = world.spawn();
        Commands commands{world};
        commands.add(entity, Tracked{.name = "shield"});
        EXPECT_EQ(liveTracked, 1);  // the recorded value only
        commands.flush();
        EXPECT_EQ(world.getComponents().get<Tracked>(entity)->name, "shield");
        EXPECT_EQ(liveTracked, 1);  // moved into the pool; the moved-from value was destroyed
    }
    EXPECT_EQ(liveTracked, 0);
}

TEST(Commands, DiscardingDestroysValuesAndCancelsSpawns) {
    liveTracked = 0;
    World world;
    const Entity entity = world.spawn();
    Entity reserved;
    {
        Commands commands{world};
        reserved = commands.spawn().getEntity();
        commands.add(entity, Tracked{.name = "lost"});
        commands.add(reserved, Tracked{.name = "lost too"});
        EXPECT_EQ(liveTracked, 2);
    }  // destroyed without a flush
    EXPECT_EQ(liveTracked, 0);
    EXPECT_FALSE(world.getComponents().has<Tracked>(entity));
    world.flushReserved();
    EXPECT_FALSE(world.isAlive(reserved));  // cancelled: never becomes alive
    EXPECT_EQ(world.getEntityCount(), 1U);
}

TEST(Commands, ClearDiscardsAndTheBufferStaysUsable) {
    liveTracked = 0;
    World world;
    const Entity entity = world.spawn();
    Commands commands{world};
    const Entity reserved = commands.spawn().getEntity();
    commands.add(entity, Tracked{.name = "lost"});
    commands.clear();
    EXPECT_TRUE(commands.isEmpty());
    EXPECT_EQ(liveTracked, 0);
    EXPECT_FALSE(world.isAlive(reserved));

    commands.add(entity, Health{.value = 4});
    commands.flush();
    EXPECT_EQ(world.getComponents().get<Health>(entity)->value, 4);
}

TEST(Commands, DiscardDespawnsAnEntityAnotherFlushMadeAlive) {
    World world;
    Commands first{world};
    Commands second{world};
    const Entity orphan = first.spawn().getEntity();
    second.flush();  // flushes every reservation of the world, including `first`'s
    EXPECT_TRUE(world.isAlive(orphan));
    first.clear();
    EXPECT_FALSE(world.isAlive(orphan));
}

TEST(Commands, MovingLeavesTheSourceEmpty) {
    World world;
    const Entity entity = world.spawn();
    Commands source{world};
    const Entity reserved = source.spawn().getEntity();
    source.add(entity, Health{.value = 5});
    Commands target{std::move(source)};
    EXPECT_TRUE(source.isEmpty());  // NOLINT(bugprone-use-after-move): checking the moved-from state is the point
    source.clear();                 // must not cancel the reservation `target` now owns
    target.flush();
    EXPECT_TRUE(world.isAlive(reserved));
    EXPECT_EQ(world.getComponents().get<Health>(entity)->value, 5);
}

TEST(Commands, MoveAssignmentDiscardsTheTarget) {
    liveTracked = 0;
    World world;
    const Entity entity = world.spawn();
    Commands target{world};
    const Entity dropped = target.spawn().getEntity();
    target.add(entity, Tracked{.name = "overwritten"});
    Commands source{world};
    source.add(entity, Health{.value = 6});
    target = std::move(source);
    EXPECT_EQ(liveTracked, 0);  // the target's pending value was destroyed
    target.flush();
    EXPECT_FALSE(world.isAlive(dropped));  // and its reservation cancelled
    EXPECT_FALSE(world.getComponents().has<Tracked>(entity));
    EXPECT_EQ(world.getComponents().get<Health>(entity)->value, 6);
}

TEST(Commands, AddsTags) {
    World world;
    const Entity entity = world.spawn();
    Commands commands{world};
    commands.add(entity, Dead{});
    commands.flush();
    EXPECT_TRUE(world.getComponents().has<Dead>(entity));
}
