/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsPluginTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/time/Time.hpp"

using rtype::plugins::physics::PhysicsPlugin;
using rtype::plugins::physics::PhysicsSettings;
using rtype::plugins::physics::PhysicsWorld;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;

/// @brief Small capacities, so tests build worlds quickly.
PhysicsSettings smallSettings() {
    return PhysicsSettings{
        .maxBodies = 16, .maxBodyPairs = 16, .maxContactConstraints = 16, .tempAllocatorBytes = 1024 * 1024};
}
}  // namespace

TEST(PhysicsPlugin, IsNamedRtypePhysics) { EXPECT_EQ(PhysicsPlugin{}.getName(), "rtype.Physics"); }

TEST(PhysicsPlugin, RegistersTheComponents) {
    rtype::ecs::App app;
    app.addPlugin(PhysicsPlugin{smallSettings()});
    EXPECT_TRUE(app.getWorld().getComponents().find("rtype.RigidBody"));
    EXPECT_TRUE(app.getWorld().getComponents().find("rtype.Collider"));
    EXPECT_TRUE(app.getWorld().getComponents().find("rtype.Velocity"));
}

TEST(PhysicsPlugin, InsertsTheWorldFromItsSettings) {
    rtype::ecs::App app;
    PhysicsSettings settings = smallSettings();
    settings.unitsPerMeter = 50.0F;
    app.addPlugin(PhysicsPlugin{settings});

    const PhysicsWorld* const world = app.getWorld().getResources().get<PhysicsWorld>();
    ASSERT_NE(world, nullptr);
    EXPECT_FLOAT_EQ(world->getUnitsPerMeter(), 50.0F);
}

TEST(PhysicsPlugin, AddsTheStepSystemInFixedUpdate) {
    rtype::ecs::App app;
    app.addPlugin(PhysicsPlugin{smallSettings()});
    EXPECT_TRUE(app.getSchedules().getSchedule(rtype::ecs::Phase::kFixedUpdate).contains(PhysicsPlugin::kStepSystem));
}

TEST(PhysicsPlugin, StepsTheWorldOncePerFixedUpdate) {
    rtype::ecs::App app;
    app.addPlugin(PhysicsPlugin{smallSettings()});
    app.getWorld().getResources().insert(rtype::ecs::Time{.fixedDelta = kFixedDelta});

    app.getSchedules().run(rtype::ecs::Phase::kFixedUpdate);
    app.getSchedules().run(rtype::ecs::Phase::kFixedUpdate);

    EXPECT_EQ(app.getWorld().getResources().require<PhysicsWorld>().getStepCount(), 2U);
}

TEST(PhysicsPlugin, RejectsInvalidSettingsWhenBuilt) {
    rtype::ecs::App app;
    PhysicsSettings settings = smallSettings();
    settings.unitsPerMeter = 0.0F;
    EXPECT_THROW(app.addPlugin(PhysicsPlugin{settings}), rtype::plugins::physics::exceptions::PhysicsException);
}
