/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldTests
*/

#include <gtest/gtest.h>

#include <array>
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

struct Bullet {
    rtype::ecs::Entity owner;
    float speed;
};
RTYPE_ECS_COMPONENT(Bullet, "test.world.Bullet", owner, speed);

struct Squad {
    std::array<rtype::ecs::Entity, 3> members;
};
RTYPE_ECS_COMPONENT(Squad, "test.world.Squad", members);
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
        .type = {.name = "test.world.Brain",
                 .size = 8,
                 .alignment = 4,
                 .fields = {{.name = "health", .type = rtype::ecs::FieldType::kI32, .offset = 0},
                            {.name = "iq", .type = rtype::ecs::FieldType::kF32, .offset = 4}}}};
}
}  // namespace

TEST(World, StartsEmpty) {
    const World world;
    EXPECT_EQ(world.getEntityCount(), 0U);
    EXPECT_EQ(world.getComponents().getRegistry().getCount(), 0U);
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
    const ComponentId position = world.getComponents().registerType<Position>();
    const ComponentId health = world.getComponents().registerType<Health>();
    EXPECT_NE(position, health);
    EXPECT_EQ(world.getComponents().registerType<Position>(), position);  // same options: same id
    EXPECT_EQ(world.getComponents().getRegistry().getCount(), 2U);
    EXPECT_EQ(world.getComponents().find("test.world.Position"), std::optional<ComponentId>{position});
}

TEST(World, ComponentThrowsWhenTheOptionsConflict) {
    World world;
    world.getComponents().registerType<Position>();
    EXPECT_THROW(world.getComponents().registerType<Position>(
                     rtype::ecs::ComponentOptions{.replication = rtype::ecs::ReplicationMode::kEveryChange}),
                 rtype::ecs::exceptions::ComponentRegistrationException);
}

TEST(World, RegistersRunTimeDescriptions) {
    World world;
    const rtype::ecs::Result<ComponentId> id = world.getComponents().registerType(makeBrain());
    ASSERT_TRUE(id);
    EXPECT_EQ(world.getComponents().find("test.world.Brain"), std::optional<ComponentId>{*id});
    const rtype::ecs::Result<ComponentId> again = world.getComponents().registerType(makeBrain());
    ASSERT_TRUE(again);
    EXPECT_EQ(*again, *id);
}

TEST(World, RefusesAnInvalidDescription) {
    World world;
    rtype::ecs::ComponentInfo brain = makeBrain();
    brain.type.alignment = 3;
    const rtype::ecs::Result<ComponentId> id = world.getComponents().registerType(std::move(brain));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kInvalidType);
    EXPECT_FALSE(world.getComponents().find("test.world.Brain"));
}

TEST(World, DoesNotFindUnknownComponents) {
    const World world;
    EXPECT_FALSE(world.getComponents().find("test.world.Unknown"));
}

TEST(World, WorldsAreIndependent) {
    World server;
    World client;
    server.getComponents().registerType<Position>();
    const ComponentId serverHealth = server.getComponents().registerType<Health>();
    const ComponentId clientHealth = client.getComponents().registerType<Health>();  // registered first here
    EXPECT_NE(serverHealth, clientHealth);                                           // ids are per world
    const Entity entity = server.spawn();
    EXPECT_TRUE(server.isAlive(entity));
    EXPECT_EQ(client.getEntityCount(), 0U);
    EXPECT_FALSE(client.getComponents().find("test.world.Position"));
}

TEST(World, MovingKeepsEntitiesAndComponents) {
    World world;
    const Entity entity = world.spawn();
    const ComponentId position = world.getComponents().registerType<Position>();
    World moved{std::move(world)};
    EXPECT_TRUE(moved.isAlive(entity));
    EXPECT_EQ(moved.getComponents().find("test.world.Position"), std::optional<ComponentId>{position});
    World assigned;
    assigned = std::move(moved);
    EXPECT_TRUE(assigned.isAlive(entity));
    EXPECT_EQ(assigned.getComponents().registerType<Position>(), position);
}

TEST(World, AddsAndReadsARunTimeComponentById) {
    World world;
    const ComponentId brain = *world.getComponents().registerType(makeBrain());
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.getComponents().add(entity, brain, bytesOf(BrainValue{.health = 90, .iq = 1.5F})));
    EXPECT_TRUE(world.getComponents().has(entity, brain));
    const std::span<const std::byte> bytes = world.getComponents().get(entity, brain);
    ASSERT_EQ(bytes.size(), sizeof(BrainValue));
    EXPECT_EQ(rtype::ecs::storage::viewAs<BrainValue>(bytes)->health, 90);
    EXPECT_EQ(rtype::ecs::storage::viewAs<BrainValue>(bytes)->iq, 1.5F);
}

TEST(World, AddReplacesTheValueOfAComponentTheEntityHas) {
    World world;
    const ComponentId brain = *world.getComponents().registerType(makeBrain());
    const Entity first = world.spawn();
    const Entity second = world.spawn();
    world.getComponents().add(first, brain, bytesOf(BrainValue{.health = 1, .iq = 0.0F}));
    world.getComponents().add(second, brain, bytesOf(BrainValue{.health = 2, .iq = 0.0F}));
    EXPECT_TRUE(world.getComponents().add(first, brain, bytesOf(BrainValue{.health = 10, .iq = 0.0F})));
    EXPECT_EQ(rtype::ecs::storage::viewAs<BrainValue>(world.getComponents().get(first, brain))->health, 10);
    EXPECT_EQ(rtype::ecs::storage::viewAs<BrainValue>(world.getComponents().get(second, brain))->health, 2);
}

TEST(World, RemovesAComponentById) {
    World world;
    const ComponentId brain = *world.getComponents().registerType(makeBrain());
    const Entity entity = world.spawn();
    world.getComponents().add(entity, brain, bytesOf(BrainValue{.health = 1, .iq = 0.0F}));
    EXPECT_TRUE(world.getComponents().remove(entity, brain));
    EXPECT_FALSE(world.getComponents().has(entity, brain));
    EXPECT_TRUE(world.getComponents().get(entity, brain).empty());
    EXPECT_FALSE(world.getComponents().remove(entity, brain));  // already removed
    EXPECT_TRUE(world.isAlive(entity));                         // removing a component keeps the entity
}

TEST(World, RefusesInvalidAdds) {
    World world;
    const ComponentId brain = *world.getComponents().registerType(makeBrain());
    const Entity entity = world.spawn();
    const BrainValue value{.health = 1, .iq = 0.0F};
    EXPECT_FALSE(world.getComponents().add(Entity{}, brain, bytesOf(value)));                      // null
    EXPECT_FALSE(world.getComponents().add(Entity{42, 1}, brain, bytesOf(value)));                 // never spawned
    EXPECT_FALSE(world.getComponents().add(Entity{entity.getIndex(), 2}, brain, bytesOf(value)));  // stale generation
    EXPECT_FALSE(world.getComponents().add(entity, brain + 1, bytesOf(value)));                    // unknown id
    EXPECT_FALSE(world.getComponents().add(entity, brain, bytesOf(std::int32_t{1})));  // 4 bytes, the component has 8
    EXPECT_FALSE(world.getComponents().has(entity, brain));
}

TEST(World, AnswersNothingForAnUnknownId) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_FALSE(world.getComponents().has(entity, 7));
    EXPECT_TRUE(world.getComponents().get(entity, 7).empty());
    EXPECT_FALSE(world.getComponents().remove(entity, 7));
}

TEST(World, AddsTagsById) {
    World world;
    const ComponentId dead =
        *world.getComponents().registerType(rtype::ecs::ComponentInfo{.type = {.name = "test.world.Dead"}});
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.getComponents().add(entity, dead, {}));
    EXPECT_TRUE(world.getComponents().has(entity, dead));
    EXPECT_TRUE(world.getComponents().get(entity, dead).empty());
}

TEST(World, CopiesNonTrivialValuesById) {
    liveCounters = 0;
    {
        World world;
        const ComponentId inventory = world.getComponents().registerType<rtype::test::world::Inventory>();
        const Entity entity = world.spawn();
        const rtype::test::world::Inventory source{.items = {1, 2}};
        EXPECT_TRUE(world.getComponents().add(entity, inventory, bytesOf(source)));
        EXPECT_EQ(source.items.size(), 2U);  // copied, untouched
        const rtype::test::world::Inventory replacement{.items = {3}};
        world.getComponents().add(entity, inventory, bytesOf(replacement));
        EXPECT_EQ(
            rtype::ecs::storage::viewAs<rtype::test::world::Inventory>(world.getComponents().get(entity, inventory))
                ->items,
            (std::vector<std::int32_t>{3}));
        EXPECT_EQ(liveCounters, 3);  // source, replacement, the stored copy
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(World, AddsAndGetsCppComponents) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.getComponents().add(entity, Position{.x = 1.0F, .y = 2.0F}));
    EXPECT_TRUE(world.getComponents().has<Position>(entity));
    const Position* position = world.getComponents().get<Position>(entity);
    ASSERT_NE(position, nullptr);
    EXPECT_EQ(position->x, 1.0F);
    EXPECT_EQ(position->y, 2.0F);
    EXPECT_EQ(world.getComponents().getRegistry().getCount(), 1U);  // registered by the first add
}

TEST(World, AddOnADeadEntityRegistersNothing) {
    World world;
    EXPECT_FALSE(world.getComponents().add(Entity{3, 1}, Position{.x = 0.0F, .y = 0.0F}));
    EXPECT_EQ(world.getComponents().getRegistry().getCount(), 0U);
}

TEST(World, AddReplacesACppValue) {
    World world;
    const Entity entity = world.spawn();
    world.getComponents().add(entity, Health{.value = 10});
    EXPECT_TRUE(world.getComponents().add(entity, Health{.value = 3}));
    EXPECT_EQ(world.getComponents().get<Health>(entity)->value, 3);
}

TEST(World, AnswersNothingForAnUnregisteredType) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_FALSE(world.getComponents().has<Health>(entity));
    EXPECT_EQ(world.getComponents().get<Health>(entity), nullptr);
    EXPECT_FALSE(world.getComponents().remove<Health>(entity));
    EXPECT_EQ(world.getComponents().getRegistry().getCount(), 0U);  // reading never registers
}

TEST(World, GetReturnsNullForAnEntityWithoutTheComponent) {
    World world;
    const Entity with = world.spawn();
    const Entity without = world.spawn();
    world.getComponents().add(with, Health{.value = 1});
    EXPECT_EQ(world.getComponents().get<Health>(without), nullptr);
    EXPECT_FALSE(world.getComponents().has<Health>(without));
}

TEST(World, RemovesACppComponent) {
    World world;
    const Entity entity = world.spawn();
    world.getComponents().add(entity, Health{.value = 1});
    EXPECT_TRUE(world.getComponents().remove<Health>(entity));
    EXPECT_FALSE(world.getComponents().has<Health>(entity));
    EXPECT_FALSE(world.getComponents().remove<Health>(entity));
}

TEST(World, TypedAndErasedCallsSeeTheSameData) {
    World world;
    const Entity first = world.spawn();
    const Entity second = world.spawn();
    world.getComponents().add(first, Health{.value = 7});
    const ComponentId health = *world.getComponents().find("test.world.Health");
    EXPECT_TRUE(world.getComponents().has(first, health));
    EXPECT_EQ(rtype::ecs::storage::viewAs<Health>(world.getComponents().get(first, health))->value, 7);
    world.getComponents().add(second, health, bytesOf(Health{.value = 8}));
    EXPECT_EQ(world.getComponents().get<Health>(second)->value, 8);
    world.getComponents().remove(first, health);
    EXPECT_FALSE(world.getComponents().has<Health>(first));
}

TEST(World, KeepsTheOptionsOfAnExplicitRegistration) {
    World world;
    const ComponentId health = world.getComponents().registerType<Health>(
        rtype::ecs::ComponentOptions{.replication = rtype::ecs::ReplicationMode::kEveryChange});
    const Entity entity = world.spawn();
    EXPECT_TRUE(
        world.getComponents().add(entity, Health{.value = 1}));  // finds the registration, does not register again
    EXPECT_EQ(world.getComponents().getRegistry().findById(health)->type.replication,
              rtype::ecs::ReplicationMode::kEveryChange);
}

TEST(World, AddsCppTags) {
    World world;
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.getComponents().add(entity, rtype::test::world::Dead{}));
    EXPECT_TRUE(world.getComponents().has<rtype::test::world::Dead>(entity));
    EXPECT_TRUE(world.getComponents().remove<rtype::test::world::Dead>(entity));
    EXPECT_FALSE(world.getComponents().has<rtype::test::world::Dead>(entity));
}

TEST(World, MovesNonTrivialCppValuesIn) {
    liveCounters = 0;
    {
        World world;
        const Entity entity = world.spawn();
        rtype::test::world::Inventory source{.items = {1, 2, 3}};
        world.getComponents().add(entity, std::move(source));
        EXPECT_EQ(world.getComponents().get<rtype::test::world::Inventory>(entity)->items,
                  (std::vector<std::int32_t>{1, 2, 3}));
        world.getComponents().add(entity, rtype::test::world::Inventory{.items = {4}});
        EXPECT_EQ(world.getComponents().get<rtype::test::world::Inventory>(entity)->items,
                  (std::vector<std::int32_t>{4}));
        EXPECT_EQ(liveCounters, 2);  // the stored value and the moved-from `source`
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(World, DespawnKillsTheEntity) {
    World world;
    const Entity entity = world.spawn();
    static_cast<void>(world.spawn());
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

TEST(World, AReservedEntityIsAliveOnlyAfterTheFlush) {
    World world;
    const Entity reserved = world.reserve();
    EXPECT_FALSE(reserved.isNull());
    EXPECT_FALSE(world.isAlive(reserved));
    EXPECT_EQ(world.getEntityCount(), 0U);
    EXPECT_FALSE(world.getComponents().add(reserved, Position{}));  // not alive yet

    world.flushReserved();
    EXPECT_TRUE(world.isAlive(reserved));
    EXPECT_EQ(world.getEntityCount(), 1U);
    EXPECT_TRUE(world.getComponents().add(reserved, Position{}));
}

TEST(World, ReservedAndSpawnedEntitiesAreDistinct) {
    World world;
    const Entity reserved = world.reserve();
    const Entity spawned = world.spawn();
    EXPECT_NE(reserved, spawned);
    world.flushReserved();
    EXPECT_TRUE(world.isAlive(reserved));
    EXPECT_TRUE(world.isAlive(spawned));
}

TEST(World, DespawnCancelsAReservedEntity) {
    World world;
    const Entity cancelled = world.reserve();
    const Entity kept = world.reserve();
    EXPECT_TRUE(world.despawn(cancelled));
    EXPECT_FALSE(world.despawn(cancelled));  // already cancelled

    world.flushReserved();
    EXPECT_FALSE(world.isAlive(cancelled));  // a cancelled entity never becomes alive
    EXPECT_TRUE(world.isAlive(kept));
    EXPECT_EQ(world.getEntityCount(), 1U);
}

TEST(World, DespawnRemovesTheEntityFromEveryPool) {
    World world;
    const ComponentId brain = *world.getComponents().registerType(makeBrain());
    const Entity doomed = world.spawn();
    const Entity survivor = world.spawn();
    for (const Entity entity : {doomed, survivor}) {
        world.getComponents().add(entity, Position{.x = 1.0F, .y = 1.0F});
        world.getComponents().add(entity, Health{.value = 5});
        world.getComponents().add(entity, rtype::test::world::Dead{});
        world.getComponents().add(entity, brain, bytesOf(BrainValue{.health = 1, .iq = 0.0F}));
    }
    world.despawn(doomed);
    for (ComponentId id = 0; id < world.getComponents().getRegistry().getCount(); ++id) {
        EXPECT_FALSE(world.getComponents().has(doomed, id)) << "component " << id;
        EXPECT_TRUE(world.getComponents().has(survivor, id)) << "component " << id;
    }
    EXPECT_EQ(world.getComponents().get<Health>(survivor)->value, 5);
}

TEST(World, AReusedIndexStartsWithoutComponents) {
    World world;
    const Entity old = world.spawn();
    world.getComponents().add(old, Health{.value = 5});
    world.despawn(old);
    const Entity reused = world.spawn();
    ASSERT_EQ(reused.getIndex(), old.getIndex());  // the freed index is reused first
    EXPECT_NE(reused, old);                        // under a new generation
    EXPECT_FALSE(world.getComponents().has<Health>(reused));
    EXPECT_TRUE(world.getComponents().add(reused, Health{.value = 9}));  // the index is free in the pool
    EXPECT_EQ(world.getComponents().get<Health>(reused)->value, 9);
    EXPECT_EQ(world.getComponents().get<Health>(old), nullptr);  // the stale handle sees nothing
    EXPECT_FALSE(world.getComponents().add(old, Health{.value = 1}));
}

TEST(World, DespawnDestroysNonTrivialValues) {
    liveCounters = 0;
    World world;
    const Entity entity = world.spawn();
    world.getComponents().add(entity, rtype::test::world::Inventory{.items = {1}});
    EXPECT_EQ(liveCounters, 1);
    world.despawn(entity);
    EXPECT_EQ(liveCounters, 0);  // destroyed by despawn, not only when the world goes away
}

TEST(World, SpawnsAnEntityWithItsComponents) {
    World world;
    const Entity ship = world.spawn(Position{.x = 10.0F, .y = 20.0F}, Health{.value = 3});
    EXPECT_TRUE(world.isAlive(ship));
    ASSERT_NE(world.getComponents().get<Position>(ship), nullptr);
    EXPECT_EQ(world.getComponents().get<Position>(ship)->y, 20.0F);
    EXPECT_EQ(world.getComponents().get<Health>(ship)->value, 3);
    EXPECT_EQ(world.getComponents().getRegistry().getCount(), 2U);  // both registered by the spawn
}

TEST(World, SpawnsWithASingleComponentAndWithTags) {
    World world;
    const Entity single = world.spawn(Health{.value = 1});
    const Entity tagged = world.spawn(Position{.x = 0.0F, .y = 0.0F}, rtype::test::world::Dead{});
    EXPECT_EQ(world.getComponents().get<Health>(single)->value, 1);
    EXPECT_TRUE(world.getComponents().has<rtype::test::world::Dead>(tagged));
    EXPECT_FALSE(world.getComponents().has<rtype::test::world::Dead>(single));
}

TEST(World, SpawnKeepsTheLaterValueOfARepeatedType) {
    World world;
    const Entity entity = world.spawn(Health{.value = 1}, Health{.value = 2});
    EXPECT_EQ(world.getComponents().get<Health>(entity)->value, 2);
}

TEST(World, SpawnMovesNonTrivialValuesIn) {
    liveCounters = 0;
    {
        World world;
        const Entity entity = world.spawn(rtype::test::world::Inventory{.items = {5, 6}}, Health{.value = 1});
        EXPECT_EQ(world.getComponents().get<rtype::test::world::Inventory>(entity)->items,
                  (std::vector<std::int32_t>{5, 6}));
        EXPECT_EQ(liveCounters, 1);  // only the stored value: the temporaries are gone
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(World, TypedCallsRefuseARunTimeComponentThatOnlySharesTheName) {
    World world;
    rtype::ecs::ComponentInfo script = makeBrain();  // 8 bytes, while Health has 4
    script.type.name = "test.world.Health";
    const ComponentId scriptId = *world.getComponents().registerType(std::move(script));
    const Entity entity = world.spawn();
    EXPECT_THROW(world.getComponents().add(entity, Health{.value = 1}),
                 rtype::ecs::exceptions::ComponentRegistrationException);
    EXPECT_FALSE(world.getComponents().has<Health>(entity));
    EXPECT_EQ(world.getComponents().get<Health>(entity), nullptr);
    EXPECT_FALSE(world.getComponents().remove<Health>(entity));
    EXPECT_FALSE(world.getComponents().has(entity, scriptId));  // nothing was written into the script's component
    EXPECT_EQ(world.getComponents().getRegistry().getCount(), 1U);
}

TEST(World, AFailedSpawnLeavesNoEntity) {
    World world;
    rtype::ecs::ComponentInfo script = makeBrain();
    script.type.name = "test.world.Health";
    static_cast<void>(world.getComponents().registerType(std::move(script)));
    // Position is added first, then Health throws: the entity must not survive with only a Position.
    EXPECT_THROW(static_cast<void>(world.spawn(Position{.x = 1.0F, .y = 1.0F}, Health{.value = 1})),
                 rtype::ecs::exceptions::ComponentRegistrationException);
    EXPECT_EQ(world.getEntityCount(), 0U);
    const ComponentId position = *world.getComponents().find("test.world.Position");
    EXPECT_FALSE(world.getComponents().has(Entity{0, 1}, position));
}

TEST(World, AdvancingReturnsTheNewChangeTick) {
    World world;
    EXPECT_EQ(world.advanceChangeTick(), 1U);
    EXPECT_EQ(world.advanceChangeTick(5), 6U);
    EXPECT_EQ(world.getChangeTick(), 6U);
}

TEST(World, TheChangeTickWrapsAround) {
    World world;
    world.advanceChangeTick(4'000'000'000);
    world.advanceChangeTick(300'000'000);
    EXPECT_EQ(world.advanceChangeTick(1'000'000), 6'032'704U);  // 4'301'000'000 - 2^32
}

TEST(World, CheckingTicksOfAnEmptyWorldIsHarmless) {
    World world;
    world.checkChangeTicks();
    world.advanceChangeTick(rtype::ecs::kCheckTickThreshold);  // runs a check through advancing
    EXPECT_EQ(world.getChangeTick(), rtype::ecs::kCheckTickThreshold);
}

TEST(World, AddingStampsBothTicksWithTheCurrentTick) {
    World world;
    world.advanceChangeTick(3);
    const Entity entity = world.spawn(Health{.value = 1});
    const std::optional<rtype::ecs::ComponentTicks> ticks = world.getComponents().getTicks<Health>(entity);
    ASSERT_TRUE(ticks);
    EXPECT_EQ(ticks->added, 3U);
    EXPECT_EQ(ticks->changed, 3U);
}

TEST(World, ReplacingStampsOnlyTheChangedTick) {
    World world;
    world.advanceChangeTick(3);
    const Entity entity = world.spawn(Health{.value = 1});
    world.advanceChangeTick(4);
    world.getComponents().add(entity, Health{.value = 2});
    const std::optional<rtype::ecs::ComponentTicks> ticks = world.getComponents().getTicks<Health>(entity);
    ASSERT_TRUE(ticks);
    EXPECT_EQ(ticks->added, 3U);
    EXPECT_EQ(ticks->changed, 7U);
    EXPECT_FALSE(ticks->isAdded(5, world.getChangeTick()));  // a system that last ran at tick 5
    EXPECT_TRUE(ticks->isChanged(5, world.getChangeTick()));
}

TEST(World, TypedAndErasedTicksAgree) {
    World world;
    world.advanceChangeTick(2);
    const Entity entity = world.spawn(Health{.value = 1}, rtype::test::world::Dead{});
    const ComponentId health = *world.getComponents().find("test.world.Health");
    EXPECT_EQ(world.getComponents().getTicks(entity, health)->changed,
              world.getComponents().getTicks<Health>(entity)->changed);
    EXPECT_EQ(world.getComponents().getTicks<rtype::test::world::Dead>(entity)->added, 2U);  // tags have ticks too
}

TEST(World, HasNoTicksForAnAbsentComponent) {
    World world;
    const Entity entity = world.spawn(Position{.x = 0.0F, .y = 0.0F});
    EXPECT_FALSE(world.getComponents().getTicks<Health>(entity));                          // not registered
    EXPECT_FALSE(world.getComponents().getTicks(entity, 42));                              // unknown id
    EXPECT_FALSE(world.getComponents().getTicks<Position>(Entity{entity.getIndex(), 9}));  // stale handle
    world.spawn(Health{.value = 1});
    EXPECT_FALSE(world.getComponents().getTicks<Health>(entity));  // registered, but not on it
}

TEST(World, ChangeDetectionSurvivesTheTickWrapping) {
    World world;
    world.advanceChangeTick();
    const Entity entity = world.spawn(Health{.value = 1});  // added and changed at tick 1
    rtype::ecs::Tick lastRun = world.advanceChangeTick();   // a system runs at tick 2 and sees it
    // Five billion ticks later, past the 32-bit wrap; the system keeps clamping its own last run, as it must.
    for (int step = 0; step < 5; ++step) {
        world.advanceChangeTick(1'000'000'000);
        rtype::ecs::clampTick(lastRun, world.getChangeTick());
    }
    const rtype::ecs::Tick thisRun = world.getChangeTick();
    const std::optional<rtype::ecs::ComponentTicks> untouched = world.getComponents().getTicks<Health>(entity);
    ASSERT_TRUE(untouched);
    EXPECT_LE(thisRun - untouched->changed, rtype::ecs::kMaxChangeAge);  // clamped by the world
    EXPECT_FALSE(untouched->isChanged(lastRun, thisRun));                // still old, not "new" after the wrap
    world.getComponents().add(entity, Health{.value = 2});               // now written
    EXPECT_TRUE(world.getComponents().getTicks<Health>(entity)->isChanged(lastRun, thisRun));
}

TEST(World, ReadingThroughMutDoesNotMarkChanged) {
    World world;
    const Entity entity = world.spawn(Health{.value = 3});  // tick 0
    world.advanceChangeTick(5);
    const rtype::ecs::Mut<Health> health = world.getComponents().getMut<Health>(entity);
    ASSERT_TRUE(health);
    EXPECT_EQ(health.get().value, 3);
    EXPECT_EQ(world.getComponents().getTicks<Health>(entity)->changed, 0U);
}

TEST(World, WritingThroughMutMarksChanged) {
    World world;
    const Entity entity = world.spawn(Health{.value = 3});
    world.advanceChangeTick(5);
    rtype::ecs::Mut<Health> health = world.getComponents().getMut<Health>(entity);
    health->value -= 1;
    EXPECT_EQ(world.getComponents().get<Health>(entity)->value, 2);  // written in place, in the pool
    EXPECT_EQ(world.getComponents().getTicks<Health>(entity)->changed, 5U);
    EXPECT_EQ(world.getComponents().getTicks<Health>(entity)->added, 0U);
}

TEST(World, MutMarksOnlyTheValuesWrittenThrough) {
    World world;
    const Entity damaged = world.spawn(Health{.value = 3});
    const Entity untouched = world.spawn(Health{.value = 3});
    world.advanceChangeTick(5);
    world.getComponents().getMut<Health>(damaged)->value = 1;
    static_cast<void>(world.getComponents().getMut<Health>(untouched).get());
    EXPECT_EQ(world.getComponents().getTicks<Health>(damaged)->changed, 5U);
    EXPECT_EQ(world.getComponents().getTicks<Health>(untouched)->changed, 0U);
}

TEST(World, GetMutIsEmptyForAnAbsentComponent) {
    World world;
    const Entity entity = world.spawn(Position{.x = 0.0F, .y = 0.0F});
    EXPECT_FALSE(world.getComponents().getMut<Health>(entity));  // not registered
    world.spawn(Health{.value = 1});
    EXPECT_FALSE(world.getComponents().getMut<Health>(entity));                          // not on this entity
    EXPECT_FALSE(world.getComponents().getMut<Position>(Entity{entity.getIndex(), 9}));  // stale handle
}

TEST(World, TypeErasedGetMutMarksOnAccess) {
    World world;
    const ComponentId brain = *world.getComponents().registerType(makeBrain());
    const Entity entity = world.spawn();
    world.getComponents().add(entity, brain, bytesOf(BrainValue{.health = 1, .iq = 0.0F}));
    world.advanceChangeTick(4);
    const std::span<std::byte> bytes = world.getComponents().getMut(entity, brain);
    ASSERT_EQ(bytes.size(), sizeof(BrainValue));
    EXPECT_EQ(world.getComponents().getTicks(entity, brain)->changed, 4U);  // marked by the access itself
    rtype::ecs::storage::viewAs<BrainValue>(bytes)->health = 7;
    EXPECT_EQ(rtype::ecs::storage::viewAs<BrainValue>(world.getComponents().get(entity, brain))->health, 7);
}

TEST(World, TypeErasedGetMutIsEmptyForAnAbsentComponent) {
    World world;
    const ComponentId brain = *world.getComponents().registerType(makeBrain());
    const Entity entity = world.spawn();
    EXPECT_TRUE(world.getComponents().getMut(entity, brain).empty());
    EXPECT_TRUE(world.getComponents().getMut(entity, brain + 1).empty());
}

TEST(World, AMutStampsTheTickItWasMadeAt) {
    World world;
    const Entity entity = world.spawn(Health{.value = 3});
    world.advanceChangeTick(5);
    rtype::ecs::Mut<Health> health = world.getComponents().getMut<Health>(entity);
    world.advanceChangeTick();  // the world moves on; the handle keeps the tick of its system run
    health->value = 0;
    EXPECT_EQ(world.getComponents().getTicks<Health>(entity)->changed, 5U);
}

TEST(World, DespawnClearsTheEntityFieldsThatPointToIt) {
    World world;
    const Entity ship = world.spawn();
    const Entity otherShip = world.spawn();
    const Entity bullet = world.spawn(rtype::test::world::Bullet{.owner = ship, .speed = 2.0F});
    const Entity otherBullet = world.spawn(rtype::test::world::Bullet{.owner = otherShip, .speed = 3.0F});
    world.despawn(ship);
    EXPECT_TRUE(world.getComponents().get<rtype::test::world::Bullet>(bullet)->owner.isNull());
    EXPECT_EQ(world.getComponents().get<rtype::test::world::Bullet>(bullet)->speed,
              2.0F);  // only the entity field changes
    EXPECT_EQ(world.getComponents().get<rtype::test::world::Bullet>(otherBullet)->owner, otherShip);
}

TEST(World, DespawnClearsEveryMatchingElementOfAnEntityArray) {
    World world;
    const Entity leader = world.spawn();
    const Entity wingman = world.spawn();
    const Entity squad = world.spawn(rtype::test::world::Squad{.members = {leader, wingman, leader}});
    world.despawn(leader);
    const std::array<Entity, 3>& members = world.getComponents().get<rtype::test::world::Squad>(squad)->members;
    EXPECT_TRUE(members[0].isNull());
    EXPECT_EQ(members[1], wingman);
    EXPECT_TRUE(members[2].isNull());
}

TEST(World, AClearedValueIsMarkedChanged) {
    World world;
    const Entity ship = world.spawn();
    const Entity bullet = world.spawn(rtype::test::world::Bullet{.owner = ship, .speed = 1.0F});
    const Entity stray = world.spawn(rtype::test::world::Bullet{.owner = Entity{}, .speed = 1.0F});
    world.advanceChangeTick(5);
    world.despawn(ship);
    EXPECT_EQ(world.getComponents().getTicks<rtype::test::world::Bullet>(bullet)->changed,
              5U);  // the network must send the null
    EXPECT_EQ(world.getComponents().getTicks<rtype::test::world::Bullet>(stray)->changed, 0U);  // untouched
}

TEST(World, DespawnClearsEntityFieldsOfRunTimeComponents) {
    World world;
    const ComponentId homing = *world.getComponents().registerType(rtype::ecs::ComponentInfo{
        .type = {.name = "test.world.Homing",
                 .size = sizeof(Entity),
                 .alignment = alignof(Entity),
                 .fields = {{.name = "target", .type = rtype::ecs::FieldType::kEntity, .offset = 0}}}});
    const Entity target = world.spawn();
    const Entity missile = world.spawn();
    world.getComponents().add(missile, homing, bytesOf(target));
    world.despawn(target);
    EXPECT_TRUE(rtype::ecs::storage::viewAs<Entity>(world.getComponents().get(missile, homing))->isNull());
}

TEST(World, DespawnLeavesStaleHandlesOfTheSameIndexAlone) {
    World world;
    const Entity first = world.spawn();
    world.despawn(first);
    const Entity second = world.spawn();  // same index, newer generation
    ASSERT_EQ(second.getIndex(), first.getIndex());
    const Entity bullet = world.spawn(rtype::test::world::Bullet{.owner = first, .speed = 1.0F});
    world.despawn(second);
    EXPECT_EQ(world.getComponents().get<rtype::test::world::Bullet>(bullet)->owner, first);  // not the despawned handle
}

TEST(World, DespawningADeadEntityChangesNoField) {
    World world;
    const Entity ship = world.spawn();
    const Entity bullet = world.spawn(rtype::test::world::Bullet{.owner = ship, .speed = 1.0F});
    EXPECT_FALSE(world.despawn(Entity{ship.getIndex(), ship.getGeneration() + 1}));
    EXPECT_EQ(world.getComponents().get<rtype::test::world::Bullet>(bullet)->owner, ship);
}
