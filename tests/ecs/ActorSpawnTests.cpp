/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorSpawnTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/Actors.hpp"
#include "rtype/ecs/actor/ComponentValues.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/exceptions/ActorException.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::actor_spawn {
struct Position {
    float x;
};
RTYPE_ECS_COMPONENT(Position, "test.spawn.Position", x);

struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.spawn.Health", value);

struct Enemy {};
RTYPE_ECS_COMPONENT(Enemy, "test.spawn.Enemy");

struct Path {
    std::vector<std::int32_t> points;
};
RTYPE_ECS_COMPONENT(Path, "test.spawn.Path");

/// @brief Registered as server-only.
struct Brain {
    std::int32_t state;
};
RTYPE_ECS_COMPONENT(Brain, "test.spawn.Brain", state);

/// @brief Registered as client-only.
struct Sparkle {
    float glow;
};
RTYPE_ECS_COMPONENT(Sparkle, "test.spawn.Sparkle", glow);
}  // namespace rtype::test::actor_spawn

namespace ps = rtype::test::actor_spawn;

namespace {
/// @brief A world with a few actors: a Bydo (Position 1, Health 3, Enemy, Path), a Turret (Health 5) and a Boss
/// (Health 50) with two Turret children at Position -40 and 40.
class ActorSpawnTest : public ::testing::Test {
  protected:
    void SetUp() override {
        rtype::ecs::Actors& actors = world.getActors();
        rtype::ecs::Components& components = world.getComponents();
        bydo = actors.add("game.Bydo");
        actors.edit(bydo).getValues().set(components, ps::Position{.x = 1.F});
        actors.edit(bydo).getValues().set(components, ps::Health{.value = 3});
        actors.edit(bydo).getValues().set(components, ps::Enemy{});
        actors.edit(bydo).getValues().set(components, ps::Path{.points = {1, 2}});
        turret = actors.add("game.Turret");
        actors.edit(turret).getValues().set(components, ps::Health{.value = 5});
        boss = actors.add("game.Boss");
        actors.edit(boss).getValues().set(components, ps::Health{.value = 50});
        actors.addChild(boss, turret, valuesWith(ps::Position{.x = -40.F}));
        actors.addChild(boss, turret, valuesWith(ps::Position{.x = 40.F}));
    }

    /// @return Values holding only `value`.
    template <typename T>
    rtype::ecs::ComponentValues valuesWith(T value) {
        rtype::ecs::ComponentValues values;
        values.set(world.getComponents(), value);
        return values;
    }

    /// @return The entity's component, or nullptr.
    template <typename T>
    const T* get(rtype::ecs::Entity entity) const {
        return world.getComponents().get<T>(entity);
    }

    rtype::ecs::World world;                                  ///< The actors' world.
    rtype::ecs::Commands commands{world};                     ///< Records the spawns.
    rtype::ecs::ActorId bydo{rtype::ecs::kInvalidActorId};    ///< A flat actor.
    rtype::ecs::ActorId turret{rtype::ecs::kInvalidActorId};  ///< A child actor.
    rtype::ecs::ActorId boss{rtype::ecs::kInvalidActorId};    ///< An actor with two children.
};
}  // namespace

TEST_F(ActorSpawnTest, SpawnsACopyOfEachValueAtTheFlush) {
    const rtype::ecs::Entity enemy = commands.spawnActor(bydo).getEntity();
    EXPECT_FALSE(world.isAlive(enemy));  // reserved until the flush
    commands.flush();
    ASSERT_TRUE(world.isAlive(enemy));
    EXPECT_FLOAT_EQ(get<ps::Position>(enemy)->x, 1.F);
    EXPECT_EQ(get<ps::Health>(enemy)->value, 3);
    EXPECT_TRUE(world.getComponents().has<ps::Enemy>(enemy));
    EXPECT_EQ(get<ps::Path>(enemy)->points, (std::vector<std::int32_t>{1, 2}));
}

TEST_F(ActorSpawnTest, SpawnsByName) {
    const rtype::ecs::Entity enemy = commands.spawnActor("game.Bydo").getEntity();
    commands.flush();
    EXPECT_EQ(get<ps::Health>(enemy)->value, 3);
}

TEST_F(ActorSpawnTest, OverridesAValueForOneEntityOnly) {
    const rtype::ecs::Entity moved = commands.spawnActor(bydo).add(ps::Position{.x = 820.F}).getEntity();
    const rtype::ecs::Entity plain = commands.spawnActor(bydo).getEntity();
    commands.flush();
    EXPECT_FLOAT_EQ(get<ps::Position>(moved)->x, 820.F);
    EXPECT_FLOAT_EQ(get<ps::Position>(plain)->x, 1.F);
}

TEST_F(ActorSpawnTest, EachSpawnIsAnIndependentCopy) {
    const rtype::ecs::Entity first = commands.spawnActor(bydo).getEntity();
    const rtype::ecs::Entity second = commands.spawnActor(bydo).getEntity();
    commands.flush();
    world.getComponents().getMut<ps::Path>(first)->points.push_back(3);
    world.getActors().edit(bydo).getValues().set(world.getComponents(), ps::Health{.value = 99});
    EXPECT_EQ(get<ps::Path>(second)->points, (std::vector<std::int32_t>{1, 2}));
    EXPECT_EQ(get<ps::Health>(first)->value, 3);  // a later edit only changes later spawns
    const rtype::ecs::Entity third = commands.spawnActor(bydo).getEntity();
    commands.flush();
    EXPECT_EQ(get<ps::Health>(third)->value, 99);
}

TEST_F(ActorSpawnTest, SpawnsTheChildrenParentedToTheRoot) {
    const rtype::ecs::Entity root = commands.spawnActor(boss).getEntity();
    commands.flush();
    ASSERT_NE(get<rtype::ecs::Children>(root), nullptr);
    const std::vector<rtype::ecs::Entity> children = get<rtype::ecs::Children>(root)->entities;
    ASSERT_EQ(children.size(), 2U);
    EXPECT_EQ(get<ps::Health>(root)->value, 50);
    for (const rtype::ecs::Entity child : children) {
        ASSERT_NE(get<rtype::ecs::Parent>(child), nullptr);
        EXPECT_EQ(get<rtype::ecs::Parent>(child)->entity, root);
        EXPECT_EQ(get<ps::Health>(child)->value, 5);  // the child actor's values
    }
    EXPECT_FLOAT_EQ(get<ps::Position>(children[0])->x, -40.F);  // and each child's overrides, in order
    EXPECT_FLOAT_EQ(get<ps::Position>(children[1])->x, 40.F);
}

TEST_F(ActorSpawnTest, SpawnsNestedChildren) {
    const rtype::ecs::ActorId fleet = world.getActors().add("game.Fleet");
    world.getActors().addChild(fleet, boss);
    const rtype::ecs::Entity root = commands.spawnActor(fleet).getEntity();
    commands.flush();
    const rtype::ecs::Entity bossEntity = get<rtype::ecs::Children>(root)->entities.at(0);
    EXPECT_EQ(get<ps::Health>(bossEntity)->value, 50);
    EXPECT_EQ(get<rtype::ecs::Children>(bossEntity)->entities.size(), 2U);
    EXPECT_EQ(world.getEntityCount(), 4U);
}

TEST_F(ActorSpawnTest, RefusesAnUnknownActorAndRecordsNothing) {
    try {
        static_cast<void>(commands.spawnActor(42));
        FAIL() << "no ActorException";
    } catch (const rtype::ecs::exceptions::ActorException& error) {
        EXPECT_EQ(std::string{error.what()}, "No actor has the id 42");
    }
    try {
        static_cast<void>(commands.spawnActor("game.Missing"));
        FAIL() << "no ActorException";
    } catch (const rtype::ecs::exceptions::ActorException& error) {
        EXPECT_EQ(std::string{error.what()}, "No actor is named 'game.Missing'");
    }
    EXPECT_TRUE(commands.isEmpty());
}

TEST_F(ActorSpawnTest, LeavesOutTheComponentsOfTheOtherSide) {
    rtype::ecs::Components& components = world.getComponents();
    static_cast<void>(components.registerType<ps::Brain>({.presence = rtype::ecs::Presence::kServerOnly}));
    static_cast<void>(components.registerType<ps::Sparkle>({.presence = rtype::ecs::Presence::kClientOnly}));
    const rtype::ecs::ActorId drone = world.getActors().add("game.Drone");
    world.getActors().edit(drone).getValues().set(components, ps::Brain{.state = 1});
    world.getActors().edit(drone).getValues().set(components, ps::Sparkle{.glow = 1.F});
    world.getActors().addChild(boss, drone, valuesWith(ps::Brain{.state = 2}));  // overrides are filtered too

    world.getActors().setRunMode(rtype::ecs::RunMode::kClient);
    const rtype::ecs::Entity onClient = commands.spawnActor(drone).getEntity();
    world.getActors().setRunMode(rtype::ecs::RunMode::kServer);
    const rtype::ecs::Entity onServer = commands.spawnActor(drone).getEntity();
    world.getActors().setRunMode(rtype::ecs::RunMode::kStandalone);
    const rtype::ecs::Entity alone = commands.spawnActor(drone).getEntity();
    world.getActors().setRunMode(rtype::ecs::RunMode::kClient);
    const rtype::ecs::Entity bossOnClient = commands.spawnActor(boss).getEntity();
    commands.flush();

    EXPECT_FALSE(components.has<ps::Brain>(onClient));
    EXPECT_TRUE(components.has<ps::Sparkle>(onClient));
    EXPECT_TRUE(components.has<ps::Brain>(onServer));
    EXPECT_FALSE(components.has<ps::Sparkle>(onServer));
    EXPECT_TRUE(components.has<ps::Brain>(alone));
    EXPECT_TRUE(components.has<ps::Sparkle>(alone));
    const rtype::ecs::Entity droneChild = get<rtype::ecs::Children>(bossOnClient)->entities.at(2);
    EXPECT_FALSE(components.has<ps::Brain>(droneChild));
}
