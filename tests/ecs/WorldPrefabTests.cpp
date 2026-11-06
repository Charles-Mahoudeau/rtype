/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldPrefabTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/exceptions/PrefabException.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/prefab/ComponentValues.hpp"
#include "rtype/ecs/prefab/Prefab.hpp"
#include "rtype/ecs/prefab/Prefabs.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::world_prefab {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.worldprefab.Health", value);

struct Wing {
    float span;
};
RTYPE_ECS_COMPONENT(Wing, "test.worldprefab.Wing", span);

/// @brief Registered as server-only.
struct Brain {
    std::int32_t state;
};
RTYPE_ECS_COMPONENT(Brain, "test.worldprefab.Brain", state);
}  // namespace rtype::test::world_prefab

namespace wp = rtype::test::world_prefab;

namespace {
/// @return A ship prefab (Health 10) with one Wing child (span 3, overridden to 4).
rtype::ecs::PrefabId addShip(rtype::ecs::World& world) {
    rtype::ecs::Prefabs& prefabs = world.getPrefabs();
    const rtype::ecs::PrefabId wing = prefabs.add("game.Wing");
    prefabs.edit(wing).getValues().set(world.getComponents(), wp::Wing{.span = 3.F});
    const rtype::ecs::PrefabId ship = prefabs.add("game.Ship");
    prefabs.edit(ship).getValues().set(world.getComponents(), wp::Health{.value = 10});
    rtype::ecs::ComponentValues overrides;
    overrides.set(world.getComponents(), wp::Wing{.span = 4.F});
    prefabs.addChild(ship, wing, std::move(overrides));
    return ship;
}
}  // namespace

TEST(WorldPrefab, SpawnsATreeAtOnce) {
    rtype::ecs::World world;
    const rtype::ecs::Entity ship = world.spawnPrefab(addShip(world));
    ASSERT_TRUE(world.isAlive(ship));
    EXPECT_EQ(world.getComponents().get<wp::Health>(ship)->value, 10);
    ASSERT_NE(world.getComponents().get<rtype::ecs::Children>(ship), nullptr);
    const rtype::ecs::Entity wing = world.getComponents().get<rtype::ecs::Children>(ship)->entities.at(0);
    EXPECT_EQ(world.getComponents().get<rtype::ecs::Parent>(wing)->entity, ship);
    EXPECT_FLOAT_EQ(world.getComponents().get<wp::Wing>(wing)->span, 4.F);
    EXPECT_EQ(world.getEntityCount(), 2U);
}

TEST(WorldPrefab, SpawnsByNameAndRefusesAnUnknownOne) {
    rtype::ecs::World world;
    static_cast<void>(addShip(world));
    EXPECT_EQ(world.getComponents().get<wp::Health>(world.spawnPrefab("game.Ship"))->value, 10);
    try {
        static_cast<void>(world.spawnPrefab("game.Missing"));
        FAIL() << "no PrefabException";
    } catch (const rtype::ecs::exceptions::PrefabException& error) {
        EXPECT_EQ(std::string{error.what()}, "No prefab is named 'game.Missing'");
    }
    EXPECT_THROW(static_cast<void>(world.spawnPrefab(rtype::ecs::PrefabId{9})),
                 rtype::ecs::exceptions::PrefabException);
    EXPECT_EQ(world.getEntityCount(), 2U);
}

TEST(WorldPrefab, LeavesOutTheComponentsOfTheOtherSide) {
    rtype::ecs::World world;
    static_cast<void>(world.getComponents().registerType<wp::Brain>({.presence = rtype::ecs::Presence::kServerOnly}));
    const rtype::ecs::PrefabId drone = world.getPrefabs().add("game.Drone");
    world.getPrefabs().edit(drone).getValues().set(world.getComponents(), wp::Brain{.state = 1});
    world.getPrefabs().setRunMode(rtype::ecs::RunMode::kClient);
    EXPECT_FALSE(world.getComponents().has<wp::Brain>(world.spawnPrefab(drone)));
    world.getPrefabs().setRunMode(rtype::ecs::RunMode::kServer);
    EXPECT_TRUE(world.getComponents().has<wp::Brain>(world.spawnPrefab(drone)));
}
