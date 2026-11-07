/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NodeConfigTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <string>
#include <string_view>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/NodeConfig.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace rtype::test::node_config {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.nodeconfig.Health", value);

struct Position {
    float x;
};
RTYPE_ECS_COMPONENT(Position, "test.nodeconfig.Position", x);
}  // namespace rtype::test::node_config

namespace capsule = rtype::plugins::capsule;
namespace tn = rtype::test::node_config;

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

/// @brief An App with the capsule plugin.
class NodeConfigTest : public ::testing::Test {
  protected:
    void SetUp() override { app.addPlugin(capsule::CapsulePlugin{}); }

    /// @return The capsule at `path`.
    const capsule::Capsule& capsuleAt(std::string_view path) const {
        return *app.getWorld().getResources().require<capsule::Capsules>().find(path);
    }

    rtype::ecs::App app;  ///< The App the capsules are added to.
};
}  // namespace

TEST_F(NodeConfigTest, FillsTheRootAndTheChildrenInOrder) {
    capsule::NodeConfig boss = capsule::addCapsule(app, "actors/boss.actor");
    boss.with(tn::Health{.value = 50});
    boss.child("LeftTurret").instance("actors/turret.actor").with(tn::Position{.x = -40.F});
    boss.child("RightTurret").instance("actors/turret.actor").with(tn::Position{.x = 40.F}).child("Muzzle");
    boss.child("Shadow").onSide(rtype::ecs::Side::kClient);

    const auto nodes = capsuleAt("actors/boss.actor").getNodes();
    const rtype::ecs::Components& components = app.getWorld().getComponents();
    ASSERT_EQ(nodes.size(), 5U);
    EXPECT_EQ(nodes[0].values.get<tn::Health>(components)->value, 50);
    EXPECT_EQ(nodes[1].name, "LeftTurret");
    EXPECT_EQ(nodes[1].parent, 0U);
    ASSERT_TRUE(nodes[1].instance);
    EXPECT_EQ(nodes[1].instance->id, capsule::CapsuleId::fromPath("actors/turret.actor"));
    EXPECT_EQ(nodes[1].instance->path, "actors/turret.actor");
    EXPECT_FLOAT_EQ(nodes[1].values.get<tn::Position>(components)->x, -40.F);
    EXPECT_EQ(nodes[2].name, "RightTurret");
    EXPECT_EQ(nodes[3].name, "Muzzle");
    EXPECT_EQ(nodes[3].parent, 2U);  // under RightTurret: the chain went down
    EXPECT_FALSE(nodes[3].instance);
    EXPECT_EQ(nodes[4].side, rtype::ecs::Side::kClient);
    EXPECT_EQ(nodes[0].side, rtype::ecs::Side::kServer);
}

TEST_F(NodeConfigTest, SetsAValueByIdAndBytes) {
    const rtype::ecs::ComponentId health = app.getWorld().getComponents().registerType<tn::Health>();
    const tn::Health value{.value = 7};
    capsule::addCapsule(app, "actors/wall.actor").with(health, std::as_bytes(std::span{&value, 1}));
    EXPECT_EQ(
        capsuleAt("actors/wall.actor").getNodes()[0].values.get<tn::Health>(app.getWorld().getComponents())->value, 7);
}

TEST_F(NodeConfigTest, DefinesACapsuleAgain) {
    capsule::addCapsule(app, "actors/bydo.actor").with(tn::Health{.value = 3}).child("Gun");
    capsule::addCapsule(app, "actors/bydo.actor").with(tn::Health{.value = 4});
    const capsule::Capsule& bydo = capsuleAt("actors/bydo.actor");
    EXPECT_EQ(bydo.getVersion(), 1U);
    ASSERT_EQ(bydo.getNodes().size(), 1U);
    EXPECT_EQ(bydo.getNodes()[0].values.get<tn::Health>(app.getWorld().getComponents())->value, 4);
}

TEST_F(NodeConfigTest, RefusesASideOnTheRootAndBadValues) {
    capsule::NodeConfig root = capsule::addCapsule(app, "actors/bydo.actor");
    EXPECT_EQ(errorOf([&] { root.onSide(rtype::ecs::Side::kClient); }),
              "Capsule 'actors/bydo.actor': the root is spawned on every side");
    const std::int64_t tooBig = 0;
    const rtype::ecs::ComponentId health = app.getWorld().getComponents().registerType<tn::Health>();
    EXPECT_FALSE(errorOf([&] { root.with(health, std::as_bytes(std::span{&tooBig, 1})); }).empty());
    EXPECT_FALSE(errorOf([&] { static_cast<void>(root.child("a/b")); }).empty());
}

TEST(NodeConfig, RefusesToAddACapsuleWithoutThePlugin) {
    rtype::ecs::App app;
    EXPECT_EQ(errorOf([&] { static_cast<void>(capsule::addCapsule(app, "actors/bydo.actor")); }),
              "The world has no capsules: add the CapsulePlugin ('rtype.Capsule') first");
}
