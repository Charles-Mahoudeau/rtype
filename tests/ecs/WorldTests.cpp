/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/exceptions/ComponentExceptions.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
/// @brief Number of Counter instances alive, to detect leaks and double destruction.
int liveCounters = 0;

/// @brief Counts its own instances.
struct Counter {
    Counter() noexcept { ++liveCounters; }
    ~Counter() { --liveCounters; }
    Counter(const Counter& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(const Counter& /*other*/) noexcept = default;
    Counter(Counter&& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(Counter&& /*other*/) noexcept = default;
};
}  // namespace

namespace rtype::test::world {
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "test.world.Position", x, y);

struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.world.Health", value);

struct Inventory {
    std::vector<std::int32_t> items;
    Counter counter;
};
RTYPE_ECS_COMPONENT(Inventory, "test.world.Inventory");

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "test.world.Dead");
}  // namespace rtype::test::world

namespace {
using rtype::ecs::ComponentId;
using rtype::ecs::Entity;
using rtype::ecs::World;
using rtype::test::world::Health;
using rtype::test::world::Position;

/// @brief The C++ view of a Brain value, which only exists as a run-time description in the world.
struct BrainValue {
    std::int32_t health;
    float iq;
};

/// @return The bytes of a value, as the Luau bridge passes them.
template <typename T>
std::span<const std::byte> bytesOf(const T& value) {
    return std::as_bytes(std::span{&value, 1});
}

/// @return A run-time description, as a Luau script would provide it.
rtype::ecs::ComponentInfo makeBrain() {
    return rtype::ecs::ComponentInfo{
        .name = "test.world.Brain",
        .size = 8,
        .alignment = 4,
        .fields = {{.name = "health", .type = rtype::ecs::FieldType::kI32, .offset = 0},
                   {.name = "iq", .type = rtype::ecs::FieldType::kF32, .offset = 4}},
    };
}
}  // namespace

TEST(World, StartsEmpty) {
    const World world;
    EXPECT_EQ(world.getEntityCount(), 0U);
    EXPECT_EQ(world.getRegistry().getCount(), 0U);
    EXPECT_EQ(world.getChangeTick(), 0U);
    EXPECT_FALSE(world.isAlive(Entity{}));
    EXPECT_FALSE(world.isAlive(Entity{0, 1}));
}

TEST(World, SpawnsDistinctLiveEntities) {
    World world;
    const Entity first = world.spawn();
    const Entity second = world.spawn();
    EXPECT_NE(first, second);
    EXPECT_FALSE(first.isNull());
    EXPECT_TRUE(world.isAlive(first));
    EXPECT_TRUE(world.isAlive(second));
    EXPECT_EQ(world.getEntityCount(), 2U);
}

TEST(World, RegistersCppComponents) {
    World world;
    const ComponentId position = world.component<Position>();
    const ComponentId health = world.component<Health>();
    EXPECT_NE(position, health);
    EXPECT_EQ(world.component<Position>(), position);  // same options: same id
    EXPECT_EQ(world.getRegistry().getCount(), 2U);
    EXPECT_EQ(world.findComponent("test.world.Position"), std::optional<ComponentId>{position});
}

TEST(World, ComponentThrowsWhenTheOptionsConflict) {
    World world;
    world.component<Position>();
    EXPECT_THROW(world.component<Position>(
                     rtype::ecs::ComponentOptions{.replication = rtype::ecs::ReplicationMode::kEveryChange}),
                 rtype::ecs::exceptions::ComponentRegistrationException);
}

TEST(World, RegistersRunTimeDescriptions) {
    World world;
    const rtype::ecs::Result<ComponentId> id = world.registerComponent(makeBrain());
    ASSERT_TRUE(id);
    EXPECT_EQ(world.findComponent("test.world.Brain"), std::optional<ComponentId>{*id});
    const rtype::ecs::Result<ComponentId> again = world.registerComponent(makeBrain());
    ASSERT_TRUE(again);
    EXPECT_EQ(*again, *id);
}

TEST(World, RefusesAnInvalidDescription) {
    World world;
    rtype::ecs::ComponentInfo brain = makeBrain();
    brain.alignment = 3;
    const rtype::ecs::Result<ComponentId> id = world.registerComponent(std::move(brain));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kInvalidComponent);
    EXPECT_FALSE(world.findComponent("test.world.Brain"));
}

TEST(World, DoesNotFindUnknownComponents) {
    const World world;
    EXPECT_FALSE(world.findComponent("test.world.Unknown"));
}

TEST(World, WorldsAreIndependent) {
    World server;
    World client;
    server.component<Position>();
    const ComponentId serverHealth = server.component<Health>();
    const ComponentId clientHealth = client.component<Health>();  // registered first here
    EXPECT_NE(serverHealth, clientHealth);                        // ids are per world
    const Entity entity = server.spawn();
    EXPECT_TRUE(server.isAlive(entity));
    EXPECT_EQ(client.getEntityCount(), 0U);
    EXPECT_FALSE(client.findComponent("test.world.Position"));
}

TEST(World, MovingKeepsEntitiesAndComponents) {
    World world;
    const Entity entity = world.spawn();
    const ComponentId position = world.component<Position>();
    World moved{std::move(world)};
    EXPECT_TRUE(moved.isAlive(entity));
    EXPECT_EQ(moved.findComponent("test.world.Position"), std::optional<ComponentId>{position});
    World assigned;
    assigned = std::move(moved);
    EXPECT_TRUE(assigned.isAlive(entity));
    EXPECT_EQ(assigned.component<Position>(), position);
}

TEST(World, AddsAndReadsARunTimeComponentById) {
    World world;
    const ComponentId brain = *world.registerComponent(makeBrain());
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.add(entity, brain, bytesOf(BrainValue{.health = 90, .iq = 1.5F})));
    EXPECT_TRUE(world.has(entity, brain));
    const std::span<const std::byte> bytes = world.get(entity, brain);
    ASSERT_EQ(bytes.size(), sizeof(BrainValue));
    EXPECT_EQ(rtype::ecs::storage::viewAs<BrainValue>(bytes)->health, 90);
    EXPECT_EQ(rtype::ecs::storage::viewAs<BrainValue>(bytes)->iq, 1.5F);
}

TEST(World, AddReplacesTheValueOfAComponentTheEntityHas) {
    World world;
    const ComponentId brain = *world.registerComponent(makeBrain());
    const Entity first = world.spawn();
    const Entity second = world.spawn();
    world.add(first, brain, bytesOf(BrainValue{.health = 1, .iq = 0.0F}));
    world.add(second, brain, bytesOf(BrainValue{.health = 2, .iq = 0.0F}));
    EXPECT_TRUE(world.add(first, brain, bytesOf(BrainValue{.health = 10, .iq = 0.0F})));
    EXPECT_EQ(rtype::ecs::storage::viewAs<BrainValue>(world.get(first, brain))->health, 10);
    EXPECT_EQ(rtype::ecs::storage::viewAs<BrainValue>(world.get(second, brain))->health, 2);
}

TEST(World, RemovesAComponentById) {
    World world;
    const ComponentId brain = *world.registerComponent(makeBrain());
    const Entity entity = world.spawn();
    world.add(entity, brain, bytesOf(BrainValue{.health = 1, .iq = 0.0F}));
    EXPECT_TRUE(world.remove(entity, brain));
    EXPECT_FALSE(world.has(entity, brain));
    EXPECT_TRUE(world.get(entity, brain).empty());
    EXPECT_FALSE(world.remove(entity, brain));  // already removed
    EXPECT_TRUE(world.isAlive(entity));         // removing a component keeps the entity
}

TEST(World, RefusesInvalidAdds) {
    World world;
    const ComponentId brain = *world.registerComponent(makeBrain());
    const Entity entity = world.spawn();
    const BrainValue value{.health = 1, .iq = 0.0F};
    EXPECT_FALSE(world.add(Entity{}, brain, bytesOf(value)));                      // null
    EXPECT_FALSE(world.add(Entity{42, 1}, brain, bytesOf(value)));                 // never spawned
    EXPECT_FALSE(world.add(Entity{entity.getIndex(), 2}, brain, bytesOf(value)));  // stale generation
    EXPECT_FALSE(world.add(entity, brain + 1, bytesOf(value)));                    // unknown id
    EXPECT_FALSE(world.add(entity, brain, bytesOf(std::int32_t{1})));              // 4 bytes, the component has 8
    EXPECT_FALSE(world.has(entity, brain));
}

TEST(World, AnswersNothingForAnUnknownId) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_FALSE(world.has(entity, 7));
    EXPECT_TRUE(world.get(entity, 7).empty());
    EXPECT_FALSE(world.remove(entity, 7));
}

TEST(World, AddsTagsById) {
    World world;
    const ComponentId dead = *world.registerComponent(rtype::ecs::ComponentInfo{.name = "test.world.Dead"});
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.add(entity, dead, {}));
    EXPECT_TRUE(world.has(entity, dead));
    EXPECT_TRUE(world.get(entity, dead).empty());
}

TEST(World, CopiesNonTrivialValuesById) {
    liveCounters = 0;
    {
        World world;
        const ComponentId inventory = world.component<rtype::test::world::Inventory>();
        const Entity entity = world.spawn();
        const rtype::test::world::Inventory source{.items = {1, 2}};
        EXPECT_TRUE(world.add(entity, inventory, bytesOf(source)));
        EXPECT_EQ(source.items.size(), 2U);  // copied, untouched
        const rtype::test::world::Inventory replacement{.items = {3}};
        world.add(entity, inventory, bytesOf(replacement));
        EXPECT_EQ(rtype::ecs::storage::viewAs<rtype::test::world::Inventory>(world.get(entity, inventory))->items,
                  (std::vector<std::int32_t>{3}));
        EXPECT_EQ(liveCounters, 3);  // source, replacement, the stored copy
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(World, AddsAndGetsCppComponents) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.add(entity, Position{.x = 1.0F, .y = 2.0F}));
    EXPECT_TRUE(world.has<Position>(entity));
    const Position* position = world.get<Position>(entity);
    ASSERT_NE(position, nullptr);
    EXPECT_EQ(position->x, 1.0F);
    EXPECT_EQ(position->y, 2.0F);
    EXPECT_EQ(world.getRegistry().getCount(), 1U);  // registered by the first add
}

TEST(World, AddOnADeadEntityRegistersNothing) {
    World world;
    EXPECT_FALSE(world.add(Entity{3, 1}, Position{.x = 0.0F, .y = 0.0F}));
    EXPECT_EQ(world.getRegistry().getCount(), 0U);
}

TEST(World, AddReplacesACppValue) {
    World world;
    const Entity entity = world.spawn();
    world.add(entity, Health{.value = 10});
    EXPECT_TRUE(world.add(entity, Health{.value = 3}));
    EXPECT_EQ(world.get<Health>(entity)->value, 3);
}

TEST(World, AnswersNothingForAnUnregisteredType) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_FALSE(world.has<Health>(entity));
    EXPECT_EQ(world.get<Health>(entity), nullptr);
    EXPECT_FALSE(world.remove<Health>(entity));
    EXPECT_EQ(world.getRegistry().getCount(), 0U);  // reading never registers
}

TEST(World, GetReturnsNullForAnEntityWithoutTheComponent) {
    World world;
    const Entity with = world.spawn();
    const Entity without = world.spawn();
    world.add(with, Health{.value = 1});
    EXPECT_EQ(world.get<Health>(without), nullptr);
    EXPECT_FALSE(world.has<Health>(without));
}

TEST(World, RemovesACppComponent) {
    World world;
    const Entity entity = world.spawn();
    world.add(entity, Health{.value = 1});
    EXPECT_TRUE(world.remove<Health>(entity));
    EXPECT_FALSE(world.has<Health>(entity));
    EXPECT_FALSE(world.remove<Health>(entity));
}

TEST(World, TypedAndErasedCallsSeeTheSameData) {
    World world;
    const Entity first = world.spawn();
    const Entity second = world.spawn();
    world.add(first, Health{.value = 7});
    const ComponentId health = *world.findComponent("test.world.Health");
    EXPECT_TRUE(world.has(first, health));
    EXPECT_EQ(rtype::ecs::storage::viewAs<Health>(world.get(first, health))->value, 7);
    world.add(second, health, bytesOf(Health{.value = 8}));
    EXPECT_EQ(world.get<Health>(second)->value, 8);
    world.remove(first, health);
    EXPECT_FALSE(world.has<Health>(first));
}

TEST(World, KeepsTheOptionsOfAnExplicitRegistration) {
    World world;
    const ComponentId health =
        world.component<Health>(rtype::ecs::ComponentOptions{.replication = rtype::ecs::ReplicationMode::kEveryChange});
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.add(entity, Health{.value = 1}));  // finds the registration, does not register again
    EXPECT_EQ(world.getRegistry().findById(health)->replication, rtype::ecs::ReplicationMode::kEveryChange);
}

TEST(World, AddsCppTags) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.add(entity, rtype::test::world::Dead{}));
    EXPECT_TRUE(world.has<rtype::test::world::Dead>(entity));
    EXPECT_TRUE(world.remove<rtype::test::world::Dead>(entity));
    EXPECT_FALSE(world.has<rtype::test::world::Dead>(entity));
}

TEST(World, MovesNonTrivialCppValuesIn) {
    liveCounters = 0;
    {
        World world;
        const Entity entity = world.spawn();
        rtype::test::world::Inventory source{.items = {1, 2, 3}};
        world.add(entity, std::move(source));
        EXPECT_EQ(world.get<rtype::test::world::Inventory>(entity)->items, (std::vector<std::int32_t>{1, 2, 3}));
        world.add(entity, rtype::test::world::Inventory{.items = {4}});
        EXPECT_EQ(world.get<rtype::test::world::Inventory>(entity)->items, (std::vector<std::int32_t>{4}));
        EXPECT_EQ(liveCounters, 2);  // the stored value and the moved-from `source`
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(World, DespawnKillsTheEntity) {
    World world;
    const Entity entity = world.spawn();
    world.spawn();
    EXPECT_TRUE(world.despawn(entity));
    EXPECT_FALSE(world.isAlive(entity));
    EXPECT_EQ(world.getEntityCount(), 1U);
    EXPECT_FALSE(world.despawn(entity));  // already despawned
}

TEST(World, DespawnOfAnEntityThatIsNotAliveDoesNothing) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_FALSE(world.despawn(Entity{}));
    EXPECT_FALSE(world.despawn(Entity{42, 1}));
    EXPECT_FALSE(world.despawn(Entity{entity.getIndex(), entity.getGeneration() + 1}));
    EXPECT_TRUE(world.isAlive(entity));
}

TEST(World, DespawnRemovesTheEntityFromEveryPool) {
    World world;
    const ComponentId brain = *world.registerComponent(makeBrain());
    const Entity doomed = world.spawn();
    const Entity survivor = world.spawn();
    for (const Entity entity : {doomed, survivor}) {
        world.add(entity, Position{.x = 1.0F, .y = 1.0F});
        world.add(entity, Health{.value = 5});
        world.add(entity, rtype::test::world::Dead{});
        world.add(entity, brain, bytesOf(BrainValue{.health = 1, .iq = 0.0F}));
    }
    world.despawn(doomed);
    for (ComponentId id = 0; id < world.getRegistry().getCount(); ++id) {
        EXPECT_FALSE(world.has(doomed, id)) << "component " << id;
        EXPECT_TRUE(world.has(survivor, id)) << "component " << id;
    }
    EXPECT_EQ(world.get<Health>(survivor)->value, 5);
}

TEST(World, AReusedIndexStartsWithoutComponents) {
    World world;
    const Entity old = world.spawn();
    world.add(old, Health{.value = 5});
    world.despawn(old);
    const Entity reused = world.spawn();
    ASSERT_EQ(reused.getIndex(), old.getIndex());  // the freed index is reused first
    EXPECT_NE(reused, old);                        // under a new generation
    EXPECT_FALSE(world.has<Health>(reused));
    EXPECT_TRUE(world.add(reused, Health{.value = 9}));  // the index is free in the pool
    EXPECT_EQ(world.get<Health>(reused)->value, 9);
    EXPECT_EQ(world.get<Health>(old), nullptr);  // the stale handle sees nothing
    EXPECT_FALSE(world.add(old, Health{.value = 1}));
}

TEST(World, DespawnDestroysNonTrivialValues) {
    liveCounters = 0;
    World world;
    const Entity entity = world.spawn();
    world.add(entity, rtype::test::world::Inventory{.items = {1}});
    EXPECT_EQ(liveCounters, 1);
    world.despawn(entity);
    EXPECT_EQ(liveCounters, 0);  // destroyed by despawn, not only when the world goes away
}
