/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <utility>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/exceptions/ComponentExceptions.hpp"
#include "rtype/ecs/world/World.hpp"

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
}  // namespace rtype::test::world

namespace {
using rtype::ecs::ComponentId;
using rtype::ecs::Entity;
using rtype::ecs::World;
using rtype::test::world::Health;
using rtype::test::world::Position;

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
