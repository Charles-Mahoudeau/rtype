/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSpawnTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <string>
#include <vector>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "plugins/capsule/NodeConfig.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::capsule_spawn {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.capsulespawn.Health", value);

struct Position {
    float x;
};
RTYPE_ECS_COMPONENT(Position, "test.capsulespawn.Position", x);

struct Path {
    std::vector<std::int32_t> points;
};
RTYPE_ECS_COMPONENT(Path, "test.capsulespawn.Path");

/// @brief Registered as server-only.
struct Brain {
    std::int32_t state;
};
RTYPE_ECS_COMPONENT(Brain, "test.capsulespawn.Brain", state);

/// @brief Added by the game, not by a capsule.
struct Marker {};
RTYPE_ECS_COMPONENT(Marker, "test.capsulespawn.Marker");
}  // namespace rtype::test::capsule_spawn

namespace capsule = rtype::plugins::capsule;
namespace ts = rtype::test::capsule_spawn;

namespace {
/// @return The message of the CapsuleException `call` throws, or an empty string if it throws none.
template <typename Call>
std::string errorOf(Call call) {
    try {
        call();
    } catch (const capsule::exceptions::CapsuleException& error) {
        return error.what();
    }
    return "";
}

/// @brief An App with the capsule plugin and three capsules:
/// - "actors/turret.actor": a root (Health 5, Path {1, 2}) and a "Muzzle" child (Position 1);
/// - "actors/boss.actor": a root (Health 50), "Left" and "Right" turrets (Position -40 and 40), "Right" with an
///   "Extra" child (Position 99), and a client-side "Shadow" (Position 7);
/// - "scenes/level.scene": a root with no component, the boss at Position 600, and a client-side "Camera".
/// Standalone, the boss spawns 7 entities: its root, two turrets with their muzzles, the extra and the shadow.
class CapsuleSpawnTest : public ::testing::Test {
  protected:
    void SetUp() override {
        app.addPlugin(capsule::CapsulePlugin{});
        capsule::NodeConfig turret = capsule::addCapsule(app, "actors/turret.actor");
        turret.with(ts::Health{.value = 5}).with(ts::Path{.points = {1, 2}});
        turret.child("Muzzle").with(ts::Position{.x = 1.F});
        capsule::NodeConfig boss = capsule::addCapsule(app, "actors/boss.actor");
        boss.with(ts::Health{.value = 50});
        boss.child("Left").instance("actors/turret.actor").with(ts::Position{.x = -40.F});
        boss.child("Right")
            .instance("actors/turret.actor")
            .with(ts::Position{.x = 40.F})
            .child("Extra")
            .with(ts::Position{.x = 99.F});
        boss.child("Shadow").onSide(rtype::ecs::Side::kClient).with(ts::Position{.x = 7.F});
        capsule::NodeConfig level = capsule::addCapsule(app, "scenes/level.scene");
        level.child("Boss").instance("actors/boss.actor").with(ts::Position{.x = 600.F});
        level.child("Camera").onSide(rtype::ecs::Side::kClient);
    }

    /// @return The world.
    rtype::ecs::World& world() { return app.getWorld(); }

    /// @return The world's capsules.
    capsule::Capsules& capsules() { return *world().getResources().getMut<capsule::Capsules>(); }

    /// @return The entity's component, or nullptr.
    template <typename T>
    const T* get(rtype::ecs::Entity entity) {
        return world().getComponents().get<T>(entity);
    }

    /// @return The entity's children, none if it has no Children.
    std::vector<rtype::ecs::Entity> childrenOf(rtype::ecs::Entity entity) {
        const auto* children = get<rtype::ecs::Children>(entity);
        return children != nullptr ? children->entities : std::vector<rtype::ecs::Entity>{};
    }

    rtype::ecs::App app;  ///< The App holding the capsules.
};
}  // namespace

TEST_F(CapsuleSpawnTest, SpawnsACopyOfEachValueWithTheChildrenParented) {
    const rtype::ecs::Entity root = capsule::spawnCapsule(world(), "actors/turret.actor");
    EXPECT_EQ(get<ts::Health>(root)->value, 5);
    EXPECT_EQ(get<ts::Path>(root)->points, (std::vector<std::int32_t>{1, 2}));
    const std::vector<rtype::ecs::Entity> children = childrenOf(root);
    ASSERT_EQ(children.size(), 1U);
    EXPECT_FLOAT_EQ(get<ts::Position>(children[0])->x, 1.F);
    EXPECT_EQ(get<rtype::ecs::Parent>(children[0])->entity, root);
    const auto* instance = get<capsule::CapsuleInstance>(root);
    ASSERT_NE(instance, nullptr);
    EXPECT_EQ(instance->capsule, capsule::CapsuleId::fromPath("actors/turret.actor"));
    EXPECT_EQ(instance->version, 0U);
    EXPECT_FALSE(instance->nested);
    EXPECT_EQ(get<capsule::CapsuleInstance>(children[0]), nullptr);  // only roots
    EXPECT_EQ(world().getEntityCount(), 2U);
}

TEST_F(CapsuleSpawnTest, SpawnsNestedCapsulesWithTheNodesValuesOverTheirRoots) {
    const rtype::ecs::Entity boss = capsule::spawnCapsule(world(), "actors/boss.actor");
    EXPECT_EQ(get<ts::Health>(boss)->value, 50);
    const std::vector<rtype::ecs::Entity> children = childrenOf(boss);
    ASSERT_EQ(children.size(), 3U);
    const rtype::ecs::Entity left = children[0];
    EXPECT_EQ(get<ts::Health>(left)->value, 5);          // the turret's root
    EXPECT_FLOAT_EQ(get<ts::Position>(left)->x, -40.F);  // with the boss node's value over it
    EXPECT_TRUE(get<capsule::CapsuleInstance>(left)->nested);
    EXPECT_EQ(get<capsule::CapsuleInstance>(left)->capsule, capsule::CapsuleId::fromPath("actors/turret.actor"));
    EXPECT_EQ(childrenOf(left).size(), 1U);  // its muzzle
    const std::vector<rtype::ecs::Entity> right = childrenOf(children[1]);
    ASSERT_EQ(right.size(), 2U);
    EXPECT_FLOAT_EQ(get<ts::Position>(right[0])->x, 1.F);   // the turret's muzzle
    EXPECT_FLOAT_EQ(get<ts::Position>(right[1])->x, 99.F);  // and the boss's extra child, under the instanced root
    EXPECT_FLOAT_EQ(get<ts::Position>(children[2])->x, 7.F);
    EXPECT_EQ(world().getEntityCount(), 7U);
}

TEST_F(CapsuleSpawnTest, SpawnsALevelWhoseRootHasNoComponentOfItsOwn) {
    const rtype::ecs::Entity level = capsule::spawnCapsule(world(), "scenes/level.scene");
    EXPECT_EQ(get<ts::Position>(level), nullptr);
    const std::vector<rtype::ecs::Entity> children = childrenOf(level);
    ASSERT_EQ(children.size(), 2U);
    EXPECT_FLOAT_EQ(get<ts::Position>(children[0])->x, 600.F);
    EXPECT_EQ(childrenOf(children[0]).size(), 3U);
    EXPECT_EQ(world().getEntityCount(), 9U);
}

TEST_F(CapsuleSpawnTest, SpawnsTheNodesOfItsSideWithTheirSubtrees) {
    capsules().setRunMode(rtype::ecs::RunMode::kServer);
    const rtype::ecs::Entity onServer = capsule::spawnCapsule(world(), "actors/boss.actor");
    EXPECT_EQ(childrenOf(onServer).size(), 2U);  // no shadow
    capsules().setRunMode(rtype::ecs::RunMode::kClient);
    const rtype::ecs::Entity onClient = capsule::spawnCapsule(world(), "scenes/level.scene");
    const std::vector<rtype::ecs::Entity> children = childrenOf(onClient);
    ASSERT_EQ(children.size(), 1U);  // the camera; the boss is the server's, with everything under it
    EXPECT_EQ(get<ts::Position>(children[0]), nullptr);
    EXPECT_EQ(world().getEntityCount(), 8U);  // the server's boss (6) and the client's level (2)
}

TEST_F(CapsuleSpawnTest, LeavesOutTheComponentsOfTheOtherSide) {
    static_cast<void>(world().getComponents().registerType<ts::Brain>({.presence = rtype::ecs::Presence::kServerOnly}));
    capsule::addCapsule(app, "actors/drone.actor").with(ts::Brain{.state = 1}).with(ts::Health{.value = 1});
    capsules().setRunMode(rtype::ecs::RunMode::kClient);
    const rtype::ecs::Entity onClient = capsule::spawnCapsule(world(), "actors/drone.actor");
    EXPECT_FALSE(world().getComponents().has<ts::Brain>(onClient));
    EXPECT_EQ(get<ts::Health>(onClient)->value, 1);
    capsules().setRunMode(rtype::ecs::RunMode::kServer);
    EXPECT_TRUE(world().getComponents().has<ts::Brain>(capsule::spawnCapsule(world(), "actors/drone.actor")));
}

TEST_F(CapsuleSpawnTest, RecordsASpawnInCommands) {
    rtype::ecs::Commands commands{world()};
    const rtype::ecs::Entity boss =
        capsule::spawnCapsule(commands, capsules(), "actors/boss.actor").add(ts::Health{.value = 80}).getEntity();
    EXPECT_FALSE(world().isAlive(boss));  // reserved until the flush
    commands.flush();
    ASSERT_TRUE(world().isAlive(boss));
    EXPECT_EQ(get<ts::Health>(boss)->value, 80);  // the add replaced the capsule's value
    ASSERT_EQ(childrenOf(boss).size(), 3U);
    EXPECT_FLOAT_EQ(get<ts::Position>(childrenOf(boss)[0])->x, -40.F);
    EXPECT_EQ(world().getEntityCount(), 7U);
}

TEST_F(CapsuleSpawnTest, EachSpawnIsAnIndependentCopy) {
    const rtype::ecs::Entity first = capsule::spawnCapsule(world(), "actors/turret.actor");
    const rtype::ecs::Entity second = capsule::spawnCapsule(world(), "actors/turret.actor");
    world().getComponents().getMut<ts::Path>(first)->points.push_back(3);
    EXPECT_EQ(get<ts::Path>(second)->points, (std::vector<std::int32_t>{1, 2}));
}

TEST_F(CapsuleSpawnTest, RefusesWhatCannotBeSpawnedAndSpawnsNothing) {
    capsule::addCapsule(app, "actors/broken.actor").child("Gun").instance("actors/missing.actor");
    capsule::addCapsule(app, "loop/a.capsule").child("B").instance("loop/b.capsule");
    capsule::addCapsule(app, "loop/b.capsule").child("A").instance("loop/a.capsule");
    rtype::ecs::Commands commands{world()};
    EXPECT_EQ(errorOf([&] { static_cast<void>(capsule::spawnCapsule(commands, capsules(), "actors/missing.actor")); }),
              "No capsule at 'actors/missing.actor'");
    EXPECT_EQ(errorOf([&] { static_cast<void>(capsule::spawnCapsule(commands, capsules(), "actors/broken.actor")); }),
              "Capsule 'actors/broken.actor' spawns 'actors/missing.actor' at 'Gun', which no capsule has");
    EXPECT_EQ(errorOf([&] { static_cast<void>(capsule::spawnCapsule(commands, capsules(), "loop/a.capsule")); }),
              "Capsule 'loop/a.capsule' contains itself: 'loop/a.capsule' -> 'loop/b.capsule' -> 'loop/a.capsule'");
    EXPECT_TRUE(commands.isEmpty());
    EXPECT_FALSE(errorOf([&] { static_cast<void>(capsule::spawnCapsule(world(), "actors/broken.actor")); }).empty());
    EXPECT_FALSE(errorOf([&] {
                     static_cast<void>(capsule::spawnCapsule(world(), capsule::CapsuleId::fromPath("nowhere")));
                 }).empty());
    EXPECT_EQ(world().getEntityCount(), 0U);
}

TEST(CapsuleSpawn, RefusesAWorldWithoutCapsules) {
    rtype::ecs::World world;
    EXPECT_EQ(errorOf([&] { static_cast<void>(capsule::spawnCapsule(world, "actors/bydo.actor")); }),
              "The world has no capsules: add the CapsulePlugin ('rtype.Capsule') first");
}

TEST_F(CapsuleSpawnTest, LoadsACapsuleInPlaceOfTheOneLoadedBefore) {
    const rtype::ecs::Entity player = world().spawn(ts::Marker{});
    const rtype::ecs::Entity first = capsule::loadCapsule(world(), "scenes/level.scene");
    EXPECT_EQ(world().getResources().get<capsule::LoadedCapsule>()->root, first);
    const rtype::ecs::Entity second = capsule::loadCapsule(world(), "actors/turret.actor");
    EXPECT_FALSE(world().isAlive(first));
    EXPECT_TRUE(world().isAlive(player));  // outside the old capsule's tree
    const auto* loaded = world().getResources().get<capsule::LoadedCapsule>();
    EXPECT_EQ(loaded->root, second);
    EXPECT_EQ(loaded->capsule, capsule::CapsuleId::fromPath("actors/turret.actor"));
    EXPECT_EQ(world().getEntityCount(), 3U);     // the player and the turret
    static_cast<void>(world().despawn(second));  // the game despawned it: the next load does not mind
    static_cast<void>(capsule::loadCapsule(world(), "actors/turret.actor"));
    EXPECT_EQ(world().getEntityCount(), 3U);
}

TEST_F(CapsuleSpawnTest, RecordsALoadInCommands) {
    const rtype::ecs::Entity first = capsule::loadCapsule(world(), "actors/turret.actor");
    rtype::ecs::Commands commands{world()};
    capsule::LoadedCapsule& loaded = *world().getResources().getMut<capsule::LoadedCapsule>();
    const rtype::ecs::Entity second =
        capsule::loadCapsule(commands, capsules(), loaded, "actors/boss.actor").getEntity();
    EXPECT_EQ(loaded.root, second);
    commands.flush();
    EXPECT_FALSE(world().isAlive(first));
    EXPECT_EQ(world().getEntityCount(), 7U);
    EXPECT_FALSE(
        errorOf([&] { static_cast<void>(capsule::loadCapsule(commands, capsules(), loaded, "nowhere")); }).empty());
    EXPECT_EQ(loaded.root, second);  // unchanged by a load that failed
    EXPECT_TRUE(commands.isEmpty());
}

TEST_F(CapsuleSpawnTest, ReloadsTheNewVersionIntoTheSameRoot) {
    const rtype::ecs::Entity boss = capsule::spawnCapsule(world(), "actors/boss.actor");
    static_cast<void>(world().getComponents().add(boss, ts::Marker{}));
    const std::vector<rtype::ecs::Entity> old = childrenOf(boss);
    capsule::NodeConfig again = capsule::addCapsule(app, "actors/boss.actor");
    again.with(ts::Health{.value = 60});
    again.child("Left").instance("actors/turret.actor");
    capsule::reloadCapsule(world(), boss);

    ASSERT_TRUE(world().isAlive(boss));
    EXPECT_EQ(get<ts::Health>(boss)->value, 60);
    EXPECT_TRUE(world().getComponents().has<ts::Marker>(boss));  // the game's component stays
    EXPECT_EQ(get<capsule::CapsuleInstance>(boss)->version, 1U);
    EXPECT_EQ(childrenOf(boss).size(), 1U);
    for (const rtype::ecs::Entity child : old) {
        EXPECT_FALSE(world().isAlive(child));
    }
    EXPECT_EQ(world().getEntityCount(), 3U);  // the root, the turret and its muzzle
}

TEST_F(CapsuleSpawnTest, ReloadsEverythingSpawnedFromACapsule) {
    const rtype::ecs::Entity bossA = capsule::spawnCapsule(world(), "actors/boss.actor");
    const rtype::ecs::Entity bossB = capsule::spawnCapsule(world(), "actors/boss.actor");
    const rtype::ecs::Entity turret = capsule::spawnCapsule(world(), "actors/turret.actor");
    capsule::addCapsule(app, "actors/turret.actor").with(ts::Health{.value = 7});
    EXPECT_EQ(capsule::reloadInstances(world(), capsule::CapsuleId::fromPath("actors/turret.actor")), 3U);
    EXPECT_EQ(get<ts::Health>(turret)->value, 7);
    EXPECT_TRUE(childrenOf(turret).empty());  // the new version has no muzzle
    for (const rtype::ecs::Entity boss : {bossA, bossB}) {
        const rtype::ecs::Entity left = childrenOf(boss).at(0);
        EXPECT_EQ(get<ts::Health>(left)->value, 7);
        EXPECT_FLOAT_EQ(get<ts::Position>(left)->x, -40.F);  // reloaded through the boss: its override stays
    }
    EXPECT_EQ(world().getEntityCount(), 11U);  // 2 bosses of 5 (no muzzles) and the turret
}

TEST_F(CapsuleSpawnTest, RefusesToReloadWhatCannotBeReloaded) {
    const rtype::ecs::Entity plain = world().spawn();
    EXPECT_EQ(errorOf([&] { capsule::reloadCapsule(world(), plain); }),
              std::format("Entity {} is not the root of a capsule: it has no CapsuleInstance", plain.getIndex()));
    const rtype::ecs::Entity boss = capsule::spawnCapsule(world(), "actors/boss.actor");
    capsule::addCapsule(app, "actors/boss.actor").child("Gun").instance("actors/missing.actor");
    EXPECT_FALSE(errorOf([&] { capsule::reloadCapsule(world(), boss); }).empty());
    EXPECT_EQ(childrenOf(boss).size(), 3U);  // the old children are kept
}

TEST_F(CapsuleSpawnTest, RecordsAReloadInCommands) {
    const rtype::ecs::Entity turret = capsule::spawnCapsule(world(), "actors/turret.actor");
    capsule::addCapsule(app, "actors/turret.actor").with(ts::Health{.value = 9});
    rtype::ecs::Commands commands{world()};
    const std::vector<rtype::ecs::Entity> children = childrenOf(turret);
    capsule::reloadCapsule(commands, capsules(), turret, *get<capsule::CapsuleInstance>(turret), children);
    commands.flush();
    EXPECT_EQ(get<ts::Health>(turret)->value, 9);
    EXPECT_TRUE(childrenOf(turret).empty());
    EXPECT_EQ(get<capsule::CapsuleInstance>(turret)->version, 1U);
}
