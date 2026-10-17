/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EntityCommandsTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::entity_commands {
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "test.entity_commands.Position", x, y);

struct Velocity {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Velocity, "test.entity_commands.Velocity", x, y);

struct Shield {
    std::int32_t strength;
};
RTYPE_ECS_COMPONENT(Shield, "test.entity_commands.Shield", strength);

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "test.entity_commands.Dead");
}  // namespace rtype::test::entity_commands

namespace {
using rtype::ecs::Commands;
using rtype::ecs::Entity;
using rtype::ecs::EntityCommands;
using rtype::ecs::World;
using rtype::test::entity_commands::Dead;
using rtype::test::entity_commands::Position;
using rtype::test::entity_commands::Shield;
using rtype::test::entity_commands::Velocity;
}  // namespace

TEST(EntityCommands, SpawnsWithChainedComponents) {
    World world;
    Commands commands{world};
    const Entity bullet =
        commands.spawn().add(Position{.x = 1.F, .y = 2.F}).add(Velocity{.x = 600.F, .y = 0.F}).getEntity();
    EXPECT_EQ(commands.getCount(), 2U);
    commands.flush();
    EXPECT_TRUE(world.isAlive(bullet));
    EXPECT_EQ(world.getComponents().get<Position>(bullet)->y, 2.F);
    EXPECT_EQ(world.getComponents().get<Velocity>(bullet)->x, 600.F);
}

TEST(EntityCommands, ChainsCommandsOnAnExistingEntity) {
    World world;
    const Entity enemy = world.spawn(Shield{.strength = 20});
    Commands commands{world};
    const EntityCommands builder = commands.entity(enemy).remove<Shield>().add(Dead{});
    EXPECT_EQ(builder.getEntity(), enemy);
    commands.flush();
    EXPECT_FALSE(world.getComponents().has<Shield>(enemy));
    EXPECT_TRUE(world.getComponents().has<Dead>(enemy));
}

TEST(EntityCommands, DespawnsTheEntity) {
    World world;
    const Entity enemy = world.spawn();
    Commands commands{world};
    commands.entity(enemy).despawn();
    commands.flush();
    EXPECT_FALSE(world.isAlive(enemy));
}

TEST(EntityCommands, RecordsInTheBufferInOrder) {
    World world;
    Commands commands{world};
    EntityCommands ship = commands.spawn();
    ship.add(Shield{.strength = 1});
    commands.add(ship.getEntity(), Shield{.strength = 2});  // the builder and the buffer share one order
    ship.add(Shield{.strength = 3});
    commands.flush();
    EXPECT_EQ(world.getComponents().get<Shield>(ship.getEntity())->strength, 3);
}

TEST(EntityCommands, ACopiedBuilderRecordsInTheSameBuffer) {
    World world;
    Commands commands{world};
    const EntityCommands first = commands.spawn();
    EntityCommands copy = first;
    copy.add(Position{});
    EXPECT_EQ(copy.getEntity(), first.getEntity());
    EXPECT_EQ(commands.getCount(), 1U);
}
