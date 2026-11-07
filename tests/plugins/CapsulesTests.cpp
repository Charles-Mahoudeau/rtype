/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsulesTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::capsules {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.capsules.Health", value);
}  // namespace rtype::test::capsules

namespace capsule = rtype::plugins::capsule;
namespace tc = rtype::test::capsules;

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
}  // namespace

TEST(Capsules, StartsWithNoCapsules) {
    const capsule::Capsules capsules;
    EXPECT_EQ(capsules.getCount(), 0U);
    EXPECT_EQ(capsules.find("actors/bydo.actor"), nullptr);
    EXPECT_EQ(capsules.find(capsule::CapsuleId::fromPath("actors/bydo.actor")), nullptr);
    EXPECT_EQ(capsules.getRunMode(), rtype::ecs::RunMode::kStandalone);
}

TEST(Capsules, DefinesACapsuleWithARootOnly) {
    capsule::Capsules capsules;
    const capsule::Capsule& bydo = capsules.define("actors/./bydo.actor");
    EXPECT_EQ(bydo.getPath(), "actors/bydo.actor");  // normalized
    EXPECT_EQ(bydo.getId(), capsule::CapsuleId::fromPath("actors/bydo.actor"));
    EXPECT_EQ(bydo.getVersion(), 0U);
    ASSERT_EQ(bydo.getNodes().size(), 1U);
    EXPECT_EQ(bydo.getNodes()[0].parent, capsule::kNoParent);
    EXPECT_TRUE(bydo.getNodes()[0].name.empty());
    EXPECT_EQ(capsules.find("actors/bydo.actor"), &bydo);
    EXPECT_EQ(capsules.find(bydo.getId()), &bydo);
    EXPECT_EQ(capsules.getCount(), 1U);
}

TEST(Capsules, DefinesACapsuleAgainKeepingItsId) {
    rtype::ecs::World world;
    capsule::Capsules capsules;
    capsule::Capsule& boss = capsules.define("actors/boss.actor");
    boss.editNode(0).values.set(world.getComponents(), tc::Health{.value = 50});
    static_cast<void>(boss.addNode("Turret", 0));
    capsule::Capsule& again = capsules.define("actors/boss.actor");
    EXPECT_EQ(&again, &boss);  // the same capsule: what instances it stays valid
    EXPECT_EQ(again.getVersion(), 1U);
    ASSERT_EQ(again.getNodes().size(), 1U);  // emptied, to fill again
    EXPECT_EQ(again.getNodes()[0].values.getCount(), 0U);
    EXPECT_EQ(capsules.getCount(), 1U);
}

TEST(Capsules, RefusesAnEmptyPathAndAnUnknownId) {
    capsule::Capsules capsules;
    EXPECT_EQ(errorOf([&] { static_cast<void>(capsules.define("")); }), "A capsule needs a path");
    EXPECT_FALSE(errorOf([&] { static_cast<void>(capsules.edit(capsule::CapsuleId::fromPath("missing"))); }).empty());
}

TEST(Capsules, AddsNodesFoundByPath) {
    capsule::Capsules capsules;
    capsule::Capsule& boss = capsules.define("actors/boss.actor");
    const std::uint32_t turret = boss.addNode("Turret", 0);
    const std::uint32_t muzzle = boss.addNode("Muzzle", turret);
    EXPECT_EQ(turret, 1U);
    EXPECT_EQ(boss.getNodes()[muzzle].parent, turret);
    EXPECT_EQ(boss.findNode("Turret/Muzzle"), muzzle);
    EXPECT_EQ(boss.findNode("."), 0U);
    EXPECT_EQ(boss.findNode(""), 0U);
    EXPECT_FALSE(boss.findNode("Turret/Missing"));
    EXPECT_EQ(boss.getNodePath(muzzle), "Turret/Muzzle");
    EXPECT_EQ(boss.getNodePath(0), ".");
}

TEST(Capsules, RefusesABadNodeName) {
    capsule::Capsules capsules;
    capsule::Capsule& boss = capsules.define("actors/boss.actor");
    static_cast<void>(boss.addNode("Turret", 0));
    EXPECT_EQ(errorOf([&] { static_cast<void>(boss.addNode("Turret", 0)); }),
              "Capsule 'actors/boss.actor': '.' already has a child named 'Turret'");
    EXPECT_EQ(errorOf([&] { static_cast<void>(boss.addNode("a/b", 0)); }),
              "Capsule 'actors/boss.actor': 'a/b' cannot name a node (not empty, without '/', not '.' nor '..')");
    EXPECT_FALSE(errorOf([&] { static_cast<void>(boss.addNode("", 0)); }).empty());
    EXPECT_FALSE(errorOf([&] { static_cast<void>(boss.addNode("..", 0)); }).empty());
    EXPECT_EQ(errorOf([&] { static_cast<void>(boss.addNode("Gun", 9)); }),
              "Capsule 'actors/boss.actor' has no node 9 to add 'Gun' under");
    EXPECT_EQ(errorOf([&] { static_cast<void>(boss.editNode(5)); }),
              "Capsule 'actors/boss.actor' has 2 node(s), not a node 5");
    EXPECT_EQ(boss.getNodes().size(), 2U);
}

TEST(Capsules, SpawnsTheNodesAndComponentsOfItsSide) {
    capsule::Capsules capsules;
    const rtype::ecs::Presence server = rtype::ecs::Presence::kServerOnly;
    const rtype::ecs::Presence client = rtype::ecs::Presence::kClientOnly;
    EXPECT_TRUE(capsules.loads(rtype::ecs::Side::kServer) && capsules.loads(rtype::ecs::Side::kClient));
    EXPECT_TRUE(capsules.keeps(server) && capsules.keeps(client));  // standalone
    capsules.setRunMode(rtype::ecs::RunMode::kServer);
    EXPECT_FALSE(capsules.loads(rtype::ecs::Side::kClient));
    EXPECT_TRUE(capsules.loads(rtype::ecs::Side::kBoth));
    EXPECT_FALSE(capsules.keeps(client));
    EXPECT_TRUE(capsules.keeps(rtype::ecs::Presence::kBoth));
    capsules.setRunMode(rtype::ecs::RunMode::kClient);
    EXPECT_FALSE(capsules.loads(rtype::ecs::Side::kServer));
    EXPECT_TRUE(capsules.loads(rtype::ecs::Side::kClient));
    EXPECT_FALSE(capsules.keeps(server));
}

TEST(CapsulePlugin, InsertsTheCapsulesWithTheRunModeOfTheApp) {
    rtype::ecs::App app{rtype::ecs::RunMode::kServer};
    capsule::Capsules early;
    static_cast<void>(early.define("actors/bydo.actor"));
    app.insertResource(std::move(early));
    app.addPlugin(capsule::CapsulePlugin{});
    const auto& capsules = app.getWorld().getResources().require<capsule::Capsules>();
    EXPECT_EQ(capsules.getRunMode(), rtype::ecs::RunMode::kServer);
    EXPECT_NE(capsules.find("actors/bydo.actor"), nullptr);  // inserted before: kept
}
