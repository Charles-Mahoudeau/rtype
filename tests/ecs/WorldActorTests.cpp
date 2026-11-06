/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldActorTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/Actors.hpp"
#include "rtype/ecs/actor/ComponentValues.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/exceptions/ActorException.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::world_actor {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.worldactor.Health", value);

struct Wing {
    float span;
};
RTYPE_ECS_COMPONENT(Wing, "test.worldactor.Wing", span);

/// @brief Registered as server-only.
struct Brain {
    std::int32_t state;
};
RTYPE_ECS_COMPONENT(Brain, "test.worldactor.Brain", state);
}  // namespace rtype::test::world_actor

namespace wp = rtype::test::world_actor;

namespace {
/// @return A ship actor (Health 10) with one Wing child (span 3, overridden to 4).
rtype::ecs::ActorId addShip(rtype::ecs::World& world) {
    rtype::ecs::Actors& actors = world.getActors();
    const rtype::ecs::ActorId wing = actors.add("game.Wing");
    actors.edit(wing).getValues().set(world.getComponents(), wp::Wing{.span = 3.F});
    const rtype::ecs::ActorId ship = actors.add("game.Ship");
    actors.edit(ship).getValues().set(world.getComponents(), wp::Health{.value = 10});
    rtype::ecs::ComponentValues overrides;
    overrides.set(world.getComponents(), wp::Wing{.span = 4.F});
    actors.addChild(ship, wing, std::move(overrides));
    return ship;
}
}  // namespace

TEST(WorldActor, SpawnsATreeAtOnce) {
    rtype::ecs::World world;
    const rtype::ecs::Entity ship = world.spawnActor(addShip(world));
    ASSERT_TRUE(world.isAlive(ship));
    EXPECT_EQ(world.getComponents().get<wp::Health>(ship)->value, 10);
    ASSERT_NE(world.getComponents().get<rtype::ecs::Children>(ship), nullptr);
    const rtype::ecs::Entity wing = world.getComponents().get<rtype::ecs::Children>(ship)->entities.at(0);
    EXPECT_EQ(world.getComponents().get<rtype::ecs::Parent>(wing)->entity, ship);
    EXPECT_FLOAT_EQ(world.getComponents().get<wp::Wing>(wing)->span, 4.F);
    EXPECT_EQ(world.getEntityCount(), 2U);
}

TEST(WorldActor, SpawnsByNameAndRefusesAnUnknownOne) {
    rtype::ecs::World world;
    static_cast<void>(addShip(world));
    EXPECT_EQ(world.getComponents().get<wp::Health>(world.spawnActor("game.Ship"))->value, 10);
    try {
        static_cast<void>(world.spawnActor("game.Missing"));
        FAIL() << "no ActorException";
    } catch (const rtype::ecs::exceptions::ActorException& error) {
        EXPECT_EQ(std::string{error.what()}, "No actor is named 'game.Missing'");
    }
    EXPECT_THROW(static_cast<void>(world.spawnActor(rtype::ecs::ActorId{9})), rtype::ecs::exceptions::ActorException);
    EXPECT_EQ(world.getEntityCount(), 2U);
}

TEST(WorldActor, LeavesOutTheComponentsOfTheOtherSide) {
    rtype::ecs::World world;
    static_cast<void>(world.getComponents().registerType<wp::Brain>({.presence = rtype::ecs::Presence::kServerOnly}));
    const rtype::ecs::ActorId drone = world.getActors().add("game.Drone");
    world.getActors().edit(drone).getValues().set(world.getComponents(), wp::Brain{.state = 1});
    world.getActors().setRunMode(rtype::ecs::RunMode::kClient);
    EXPECT_FALSE(world.getComponents().has<wp::Brain>(world.spawnActor(drone)));
    world.getActors().setRunMode(rtype::ecs::RunMode::kServer);
    EXPECT_TRUE(world.getComponents().has<wp::Brain>(world.spawnActor(drone)));
}
