/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PrefabsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <string>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/exceptions/PrefabException.hpp"
#include "rtype/ecs/prefab/ComponentValues.hpp"
#include "rtype/ecs/prefab/Prefab.hpp"
#include "rtype/ecs/prefab/Prefabs.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::prefabs {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.prefabs.Health", value);
}  // namespace rtype::test::prefabs

namespace pre = rtype::test::prefabs;

namespace {
/// @return The message of the PrefabException `call` throws, or an empty string if it throws none.
template <typename Call>
std::string errorOf(Call call) {
    try {
        call();
    } catch (const rtype::ecs::exceptions::PrefabException& error) {
        return error.what();
    }
    return "";
}

/// @return A type of this presence, for the filter.
rtype::ecs::TypeInfo typeOf(rtype::ecs::Presence presence) {
    return rtype::ecs::TypeInfo{.name = "test.prefabs.Type", .presence = presence};
}
}  // namespace

TEST(Prefabs, AWorldStartsWithNoPrefabs) {
    const rtype::ecs::World world;
    EXPECT_EQ(world.getPrefabs().getCount(), 0U);
    EXPECT_EQ(world.getPrefabs().findByName("game.Bydo"), nullptr);
    EXPECT_EQ(world.getPrefabs().findById(0), nullptr);
    EXPECT_EQ(world.getPrefabs().getRunMode(), rtype::ecs::RunMode::kStandalone);
}

TEST(Prefabs, AddsAPrefabFoundByIdAndByName) {
    rtype::ecs::World world;
    const rtype::ecs::PrefabId bydo = world.getPrefabs().add("game.Bydo");
    const rtype::ecs::PrefabId bullet = world.getPrefabs().add("game.Bullet");
    EXPECT_EQ(bydo, 0U);
    EXPECT_EQ(bullet, 1U);
    ASSERT_NE(world.getPrefabs().findByName("game.Bullet"), nullptr);
    EXPECT_EQ(world.getPrefabs().findByName("game.Bullet")->getId(), bullet);
    EXPECT_EQ(world.getPrefabs().findById(bydo)->getName(), "game.Bydo");
    EXPECT_EQ(world.getPrefabs().findById(bydo)->getValues().getCount(), 0U);
    EXPECT_TRUE(world.getPrefabs().findById(bydo)->getChildren().empty());
}

TEST(Prefabs, EditsAPrefabsValues) {
    rtype::ecs::World world;
    const rtype::ecs::PrefabId bydo = world.getPrefabs().add("game.Bydo");
    world.getPrefabs().edit(bydo).getValues().set(world.getComponents(), pre::Health{.value = 3});
    const rtype::ecs::Prefab* prefab = world.getPrefabs().findById(bydo);
    ASSERT_NE(prefab->getValues().get<pre::Health>(world.getComponents()), nullptr);
    EXPECT_EQ(prefab->getValues().get<pre::Health>(world.getComponents())->value, 3);
}

TEST(Prefabs, RefusesAnEmptyOrTakenName) {
    rtype::ecs::World world;
    static_cast<void>(world.getPrefabs().add("game.Bydo"));
    EXPECT_EQ(errorOf([&] { static_cast<void>(world.getPrefabs().add("")); }), "A prefab needs a name");
    EXPECT_EQ(errorOf([&] { static_cast<void>(world.getPrefabs().add("game.Bydo")); }),
              "Prefab 'game.Bydo' is already registered");
    EXPECT_EQ(world.getPrefabs().getCount(), 1U);
}

TEST(Prefabs, RefusesToEditAnUnknownPrefab) {
    rtype::ecs::World world;
    EXPECT_EQ(errorOf([&] { static_cast<void>(world.getPrefabs().edit(4)); }), "No prefab has the id 4");
}

TEST(Prefabs, AddsChildrenWithTheirOverrides) {
    rtype::ecs::World world;
    rtype::ecs::Prefabs& prefabs = world.getPrefabs();
    const rtype::ecs::PrefabId boss = prefabs.add("game.Boss");
    const rtype::ecs::PrefabId turret = prefabs.add("game.Turret");
    rtype::ecs::ComponentValues overrides;
    overrides.set(world.getComponents(), pre::Health{.value = 9});
    prefabs.addChild(boss, turret, std::move(overrides));
    prefabs.addChild(boss, turret);
    const auto children = prefabs.findById(boss)->getChildren();
    ASSERT_EQ(children.size(), 2U);
    EXPECT_EQ(children[0].prefab, turret);
    EXPECT_EQ(children[0].overrides.get<pre::Health>(world.getComponents())->value, 9);
    EXPECT_EQ(children[1].overrides.getCount(), 0U);
}

TEST(Prefabs, RefusesAChildWithAnUnknownId) {
    rtype::ecs::World world;
    const rtype::ecs::PrefabId boss = world.getPrefabs().add("game.Boss");
    EXPECT_EQ(errorOf([&] { world.getPrefabs().addChild(boss, 7); }), "No prefab has the id 7");
    EXPECT_EQ(errorOf([&] { world.getPrefabs().addChild(7, boss); }), "No prefab has the id 7");
}

TEST(Prefabs, RefusesAPrefabContainingItself) {
    rtype::ecs::World world;
    rtype::ecs::Prefabs& prefabs = world.getPrefabs();
    const rtype::ecs::PrefabId a = prefabs.add("game.A");
    const rtype::ecs::PrefabId b = prefabs.add("game.B");
    const rtype::ecs::PrefabId c = prefabs.add("game.C");
    prefabs.addChild(a, b);
    prefabs.addChild(b, c);
    EXPECT_EQ(errorOf([&] { prefabs.addChild(a, a); }),
              "Prefab 'game.A' cannot have 'game.A' as a child: 'game.A' contains 'game.A'");
    EXPECT_EQ(errorOf([&] { prefabs.addChild(c, a); }),
              "Prefab 'game.C' cannot have 'game.A' as a child: 'game.A' contains 'game.C'");
    EXPECT_TRUE(prefabs.findById(c)->getChildren().empty());
    prefabs.addChild(a, c);  // a diamond is not a cycle
    EXPECT_EQ(prefabs.findById(a)->getChildren().size(), 2U);
}

TEST(Prefabs, KeepsTheComponentsOfItsSide) {
    rtype::ecs::Prefabs prefabs;
    const rtype::ecs::TypeInfo both = typeOf(rtype::ecs::Presence::kBoth);
    const rtype::ecs::TypeInfo server = typeOf(rtype::ecs::Presence::kServerOnly);
    const rtype::ecs::TypeInfo client = typeOf(rtype::ecs::Presence::kClientOnly);
    EXPECT_TRUE(prefabs.keeps(both) && prefabs.keeps(server) && prefabs.keeps(client));  // standalone
    prefabs.setRunMode(rtype::ecs::RunMode::kClient);
    EXPECT_TRUE(prefabs.keeps(both));
    EXPECT_FALSE(prefabs.keeps(server));
    EXPECT_TRUE(prefabs.keeps(client));
    prefabs.setRunMode(rtype::ecs::RunMode::kServer);
    EXPECT_TRUE(prefabs.keeps(both));
    EXPECT_TRUE(prefabs.keeps(server));
    EXPECT_FALSE(prefabs.keeps(client));
    EXPECT_EQ(prefabs.getRunMode(), rtype::ecs::RunMode::kServer);
}
