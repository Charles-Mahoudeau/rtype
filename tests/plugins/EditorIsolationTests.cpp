/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorIsolationTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>

#include "editor/scene/Isolation.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/graphics/Camera.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
namespace capsule = rtype::plugins::capsule;
namespace scene = rtype::editor::scene;
using rtype::plugins::transform::GlobalTransform;
using rtype::plugins::transform::Transform;

/// @brief An App with the transform and capsule plugins, and a turret at (50, 20) with a muzzle.
class IsolationTest : public ::testing::Test {
  protected:
    void SetUp() override {
        app.addPlugin(rtype::plugins::transform::TransformPlugin{});
        app.addPlugin(capsule::CapsulePlugin{});
        app.finishPlugins();
        static_cast<void>(app.getWorld().getComponents().registerType<rtype::plugins::graphics::Camera>());
        capsule::Capsule turret{"actors/turret.actor"};
        turret.editNode(0).values.set(app.getWorld().getComponents(), Transform{.position = {50.0F, 20.0F}});
        static_cast<void>(turret.addNode("Muzzle", 0));
        app.getWorld().getResources().getMut<capsule::Capsules>()->define(std::move(turret));
    }

    rtype::ecs::World& world() { return app.getWorld(); }

    const capsule::CapsuleId turretId = capsule::CapsuleId::fromPath("actors/turret.actor");
    rtype::ecs::App app;
};
}  // namespace

TEST_F(IsolationTest, SpawnsTheCapsuleUnderAnAnchorFarFromTheGame) {
    std::string error;
    const std::optional<scene::Isolation> isolation = scene::isolate(world(), turretId, error);
    ASSERT_TRUE(isolation.has_value()) << error;
    EXPECT_EQ(isolation->capsule, turretId);
    EXPECT_EQ(world().getComponents().get<Transform>(isolation->anchor)->position, scene::kIsolatedOrigin);
    EXPECT_EQ(world().getComponents().get<rtype::ecs::Parent>(isolation->root)->entity, isolation->anchor);
    EXPECT_EQ(world().getComponents().get<Transform>(isolation->root)->position, glm::vec2(50.0F, 20.0F));
}

TEST_F(IsolationTest, LooksAtTheCapsulesRoot) {
    std::string error;
    const std::optional<scene::Isolation> isolation = scene::isolate(world(), turretId, error);
    ASSERT_TRUE(isolation.has_value()) << error;
    app.runFrame();  // the global transforms
    EXPECT_EQ(scene::centerOf(world(), *isolation), scene::kIsolatedOrigin + glm::vec2(50.0F, 20.0F));
}

TEST_F(IsolationTest, DespawnsWhatItSpawnedWhenItEnds) {
    std::string error;
    const std::optional<scene::Isolation> isolation = scene::isolate(world(), turretId, error);
    ASSERT_TRUE(isolation.has_value()) << error;
    const std::size_t before = world().getEntityCount();
    EXPECT_EQ(before, 3U);  // the anchor, the turret and its muzzle
    scene::endIsolation(world(), *isolation);
    EXPECT_EQ(world().getEntityCount(), 0U);
}

TEST_F(IsolationTest, TellsWhyACapsuleCannotBeIsolated) {
    std::string error;
    EXPECT_FALSE(scene::isolate(world(), capsule::CapsuleId::fromPath("actors/missing.actor"), error).has_value());
    EXPECT_FALSE(error.empty());
}
