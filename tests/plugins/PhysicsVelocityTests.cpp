/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsVelocityTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>
#include <optional>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/time/Time.hpp"

using rtype::ecs::Entity;
using rtype::plugins::physics::BodyType;
using rtype::plugins::physics::Collider;
using rtype::plugins::physics::PhysicsPlugin;
using rtype::plugins::physics::PhysicsSettings;
using rtype::plugins::physics::PhysicsWorld;
using rtype::plugins::physics::RigidBody;
using rtype::plugins::physics::ShapeType;
using rtype::plugins::physics::Velocity;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;
constexpr float kUnitsPerMeter = 100.0F;
constexpr float kTolerance = 1e-2F;

/// @brief A body that nothing slows down.
constexpr RigidBody kFreeBody{.linearDamping = 0.0F, .angularDamping = 0.0F};

/// @brief An App with the PhysicsPlugin, 100 units per meter, ticked by hand.
struct PhysicsApp {
    rtype::ecs::App app;  ///< The App under test.

    explicit PhysicsApp(glm::vec2 gravity = {0.0F, 0.0F}) {
        app.addPlugin(PhysicsPlugin{PhysicsSettings{.gravity = gravity,
                                                    .unitsPerMeter = kUnitsPerMeter,
                                                    .maxBodies = 16,
                                                    .maxBodyPairs = 16,
                                                    .maxContactConstraints = 16,
                                                    .tempAllocatorBytes = 1024 * 1024}});
        app.getWorld().getResources().insert(rtype::ecs::Time{.fixedDelta = kFixedDelta});
    }

    /// @brief Runs the given number of fixed ticks.
    void tick(int count = 1) {
        for (int index = 0; index < count; ++index) {
            app.getSchedules().run(rtype::ecs::Phase::kFixedUpdate);
        }
    }

    [[nodiscard]] rtype::ecs::Components& components() { return app.getWorld().getComponents(); }
    [[nodiscard]] rtype::ecs::Mut<PhysicsWorld> physics() {
        return app.getWorld().getResources().requireMut<PhysicsWorld>();
    }
    [[nodiscard]] Transform transformOf(Entity entity) { return *components().get<Transform>(entity); }
    [[nodiscard]] Velocity velocityOf(Entity entity) { return *components().get<Velocity>(entity); }

    /// @return A new entity at the position with a circle Collider and, when given, a RigidBody and a Velocity.
    Entity spawn(glm::vec2 position, std::optional<RigidBody> body, std::optional<Velocity> velocity = std::nullopt) {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = position});
        components().add(entity, Collider{.shape = ShapeType::kCircle, .radius = 8.0F});
        if (body) {
            components().add(entity, *body);
        }
        if (velocity) {
            components().add(entity, *velocity);
        }
        return entity;
    }
};
}  // namespace

TEST(PhysicsVelocity, AVelocityOf300MovesABody5UnitsPerTickAt60Hz) {
    PhysicsApp fixture;
    const Entity ship = fixture.spawn({0.0F, 0.0F}, kFreeBody, Velocity{.linear = {300.0F, 0.0F}});
    fixture.tick();
    EXPECT_NEAR(fixture.transformOf(ship).position.x, 5.0F, kTolerance);
    fixture.tick(59);
    EXPECT_NEAR(fixture.transformOf(ship).position.x, 300.0F, 0.1F);
    EXPECT_NEAR(fixture.transformOf(ship).position.y, 0.0F, kTolerance);
    EXPECT_NEAR(fixture.velocityOf(ship).linear.x, 300.0F, kTolerance);
}

TEST(PhysicsVelocity, AVelocityChangedByGameplayIsGivenToTheBody) {
    PhysicsApp fixture;
    const Entity ship = fixture.spawn({0.0F, 0.0F}, kFreeBody, Velocity{});
    fixture.tick();

    fixture.components().add(ship, Velocity{.linear = {0.0F, -120.0F}, .angular = 1.5F});
    fixture.tick();
    const std::optional<Velocity> velocity = fixture.physics()->getBodyVelocity(ship);
    ASSERT_TRUE(velocity);
    EXPECT_NEAR(velocity->linear.y, -120.0F, kTolerance);
    EXPECT_NEAR(velocity->angular, 1.5F, kTolerance);
    EXPECT_NEAR(fixture.transformOf(ship).position.y, -2.0F, kTolerance);
}

TEST(PhysicsVelocity, TheVelocityOfAFallingBodyFollowsGravity) {
    PhysicsApp fixture{{0.0F, 980.0F}};
    const Entity ball = fixture.spawn({0.0F, 0.0F}, kFreeBody, Velocity{});
    fixture.tick(60);
    EXPECT_NEAR(fixture.velocityOf(ball).linear.x, 0.0F, kTolerance);
    EXPECT_NEAR(fixture.velocityOf(ball).linear.y, 980.0F, 1.0F);  // g t over one second, Y down.
}

TEST(PhysicsVelocity, DampingSlowsTheBodyDown) {
    PhysicsApp fixture;
    const Entity damped =
        fixture.spawn({0.0F, 0.0F}, RigidBody{.linearDamping = 1.0F}, Velocity{.linear = {300.0F, 0.0F}});
    const Entity free = fixture.spawn({0.0F, 100.0F}, kFreeBody, Velocity{.linear = {300.0F, 0.0F}});
    fixture.tick(60);
    EXPECT_NEAR(fixture.velocityOf(free).linear.x, 300.0F, kTolerance);
    EXPECT_GT(fixture.velocityOf(damped).linear.x, 50.0F);
    EXPECT_LT(fixture.velocityOf(damped).linear.x, 200.0F);  // About 300 / e after one second.
    EXPECT_LT(fixture.transformOf(damped).position.x, fixture.transformOf(free).position.x);
}

TEST(PhysicsVelocity, AStaticBodyIgnoresItsVelocity) {
    PhysicsApp fixture;
    const Entity wall = fixture.spawn({10.0F, 20.0F}, std::nullopt, Velocity{.linear = {300.0F, 0.0F}});
    fixture.tick(10);
    const std::optional<glm::vec2> position = fixture.physics()->getBodyPosition(wall);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->x, 10.0F, kTolerance);
    EXPECT_NEAR(position->y, 20.0F, kTolerance);
    EXPECT_FALSE(fixture.physics()->isVelocityOutOfSync(wall, Velocity{.linear = {1.0F, 0.0F}}));
}

TEST(PhysicsVelocity, AKinematicBodyMovesAtItsVelocity) {
    PhysicsApp fixture;
    const Entity ship =
        fixture.spawn({0.0F, 0.0F}, RigidBody{.type = BodyType::kKinematic}, Velocity{.linear = {0.0F, -120.0F}});
    fixture.tick(30);
    EXPECT_NEAR(fixture.transformOf(ship).position.y, -60.0F, 0.1F);
    EXPECT_NEAR(fixture.velocityOf(ship).linear.y, -120.0F, kTolerance);  // Only read for a kinematic body.
}

TEST(PhysicsVelocity, AKinematicBodyMovedByGameplayGoesBackToItsVelocity) {
    PhysicsApp fixture;
    const Entity ship =
        fixture.spawn({0.0F, 0.0F}, RigidBody{.type = BodyType::kKinematic}, Velocity{.linear = {60.0F, 0.0F}});
    fixture.tick(10);
    EXPECT_NEAR(fixture.transformOf(ship).position.x, 10.0F, kTolerance);

    fixture.components().add(ship, Transform{.position = {100.0F, 0.0F}});
    fixture.tick();
    std::optional<glm::vec2> position = fixture.physics()->getBodyPosition(ship);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->x, 100.0F, kTolerance);

    fixture.tick(60);
    position = fixture.physics()->getBodyPosition(ship);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->x, 160.0F, 0.1F);
    EXPECT_NEAR(fixture.transformOf(ship).position.x, 160.0F, 0.1F);
}

TEST(PhysicsVelocity, AnEntityWithoutVelocityGetsNone) {
    PhysicsApp fixture{{0.0F, 980.0F}};
    const Entity ball = fixture.spawn({0.0F, 0.0F}, RigidBody{});
    fixture.tick(10);
    EXPECT_EQ(fixture.components().get<Velocity>(ball), nullptr);
    const std::optional<Velocity> velocity = fixture.physics()->getBodyVelocity(ball);
    ASSERT_TRUE(velocity);
    EXPECT_GT(velocity->linear.y, 0.0F);
}

TEST(PhysicsForces, AnImpulseChangesTheVelocityByItOverTheMass) {
    PhysicsApp fixture;
    const Entity ship = fixture.spawn({0.0F, 0.0F}, RigidBody{.mass = 2.0F, .linearDamping = 0.0F}, Velocity{});
    fixture.tick();

    fixture.physics()->applyImpulse(ship, {600.0F, -200.0F});
    fixture.tick();
    EXPECT_NEAR(fixture.velocityOf(ship).linear.x, 300.0F, kTolerance);
    EXPECT_NEAR(fixture.velocityOf(ship).linear.y, -100.0F, kTolerance);
    fixture.tick(10);  // Applied once.
    EXPECT_NEAR(fixture.velocityOf(ship).linear.x, 300.0F, kTolerance);
}

TEST(PhysicsForces, AnImpulseWakesASleepingBody) {
    PhysicsApp fixture;
    const Entity ball = fixture.spawn({0.0F, 0.0F}, kFreeBody);
    fixture.tick(120);  // At rest long enough to sleep.

    fixture.physics()->applyImpulse(ball, {100.0F, 0.0F});
    fixture.tick(60);
    EXPECT_NEAR(fixture.transformOf(ball).position.x, 100.0F, 0.1F);
}

TEST(PhysicsForces, AForceOnlyPushesDuringTheNextStep) {
    PhysicsApp fixture;
    const Entity ship = fixture.spawn({0.0F, 0.0F}, kFreeBody, Velocity{});
    fixture.tick();

    fixture.physics()->applyForce(ship, {6000.0F, 0.0F});  // 60 m/s^2 on 1 kg, for 1/60 s: 100 units/s.
    fixture.tick();
    EXPECT_NEAR(fixture.velocityOf(ship).linear.x, 100.0F, kTolerance);
    fixture.tick(10);
    EXPECT_NEAR(fixture.velocityOf(ship).linear.x, 100.0F, kTolerance);
}

TEST(PhysicsForces, ForcesAddUp) {
    PhysicsApp fixture;
    const Entity ship = fixture.spawn({0.0F, 0.0F}, kFreeBody, Velocity{});
    fixture.tick();

    fixture.physics()->applyForce(ship, {6000.0F, 0.0F});
    fixture.physics()->applyForce(ship, {0.0F, 3000.0F});
    fixture.physics()->applyImpulse(ship, {-50.0F, 0.0F});
    fixture.tick();
    EXPECT_NEAR(fixture.velocityOf(ship).linear.x, 50.0F, kTolerance);
    EXPECT_NEAR(fixture.velocityOf(ship).linear.y, 50.0F, kTolerance);
}

TEST(PhysicsForces, ATorqueSpinsTheBodyClockwise) {
    PhysicsApp fixture;
    const Entity ship = fixture.spawn({0.0F, 0.0F}, kFreeBody, Velocity{});
    fixture.tick();

    fixture.physics()->applyTorque(ship, 1000.0F);
    fixture.tick();
    EXPECT_GT(fixture.velocityOf(ship).angular, 0.0F);
    fixture.tick(10);
    EXPECT_GT(fixture.transformOf(ship).rotation, 0.0F);
}

TEST(PhysicsForces, ATorqueDoesNotSpinAFixedRotationBody) {
    PhysicsApp fixture;
    const Entity ship = fixture.spawn({0.0F, 0.0F}, RigidBody{.fixedRotation = true}, Velocity{});
    fixture.tick();

    fixture.physics()->applyTorque(ship, 1000.0F);
    fixture.tick(10);
    EXPECT_FLOAT_EQ(fixture.velocityOf(ship).angular, 0.0F);
    EXPECT_FLOAT_EQ(fixture.transformOf(ship).rotation, 0.0F);
}

TEST(PhysicsForces, PushesOnBodiesThatDoNotMoveAreIgnored) {
    PhysicsApp fixture;
    const Entity wall = fixture.spawn({0.0F, 0.0F}, std::nullopt);
    const Entity platform = fixture.spawn({50.0F, 0.0F}, RigidBody{.type = BodyType::kKinematic});
    const Entity ghost = fixture.app.getWorld().spawn();
    fixture.tick();

    for (const Entity entity : {wall, platform, ghost}) {
        fixture.physics()->applyForce(entity, {6000.0F, 0.0F});
        fixture.physics()->applyImpulse(entity, {100.0F, 0.0F});
        fixture.physics()->applyTorque(entity, 1000.0F);
    }
    fixture.tick(10);
    EXPECT_NEAR(fixture.physics()->getBodyPosition(wall)->x, 0.0F, kTolerance);
    EXPECT_NEAR(fixture.physics()->getBodyPosition(platform)->x, 50.0F, kTolerance);
    EXPECT_FALSE(fixture.physics()->getBodyPosition(ghost));
}

TEST(PhysicsForces, PushesForAnEntityWithoutBodyAreForgotten) {
    PhysicsApp fixture;
    const Entity ship = fixture.app.getWorld().spawn();
    fixture.physics()->applyImpulse(ship, {100.0F, 0.0F});
    fixture.tick();  // No body yet: the impulse is dropped.

    fixture.components().add(ship, Transform{});
    fixture.components().add(ship, Collider{.shape = ShapeType::kCircle, .radius = 8.0F});
    fixture.components().add(ship, kFreeBody);
    fixture.tick(10);
    EXPECT_NEAR(fixture.transformOf(ship).position.x, 0.0F, kTolerance);
}
