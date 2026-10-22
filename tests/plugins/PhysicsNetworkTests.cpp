/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsNetworkTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/MoveAndSlide.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

using rtype::ecs::Entity;
using rtype::ecs::ReplicationMode;
using rtype::ecs::RunMode;
using rtype::plugins::physics::BodyType;
using rtype::plugins::physics::Collider;
using rtype::plugins::physics::moveAndSlide;
using rtype::plugins::physics::PhysicsHandle;
using rtype::plugins::physics::PhysicsPlugin;
using rtype::plugins::physics::PhysicsSettings;
using rtype::plugins::physics::PhysicsWorld;
using rtype::plugins::physics::QueryFilter;
using rtype::plugins::physics::RigidBody;
using rtype::plugins::physics::ShapeType;
using rtype::plugins::physics::SlideSettings;
using rtype::plugins::physics::Velocity;
using rtype::plugins::physics::debug::PhysicsDebugPlugin;
using rtype::plugins::physics::exceptions::PhysicsException;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;
constexpr float kTolerance = 0.05F;
constexpr std::uint32_t kMaxBodies = 16;
constexpr std::uint32_t kWallLayer = 1U << 1;
constexpr std::uint32_t kShipLayer = 1U << 2;

/// @brief Small capacities and a gravity, so a dynamic body falls when the world steps.
PhysicsSettings smallSettings() {
    return PhysicsSettings{.gravity = {0.0F, 980.0F},
                           .unitsPerMeter = 100.0F,
                           .maxBodies = kMaxBodies,
                           .maxBodyPairs = kMaxBodies,
                           .maxContactConstraints = kMaxBodies,
                           .tempAllocatorBytes = 1024 * 1024};
}

/// @brief The wall of every scene: static, its left face at x = 90, from y = -200 to y = 200.
Collider wallCollider() { return Collider{.shape = ShapeType::kBox, .size = {20.0F, 400.0F}, .layer = kWallLayer}; }

/// @brief The ship of every scene: a circle of radius 10 that collides with the walls.
Collider shipCollider() {
    return Collider{.shape = ShapeType::kCircle, .radius = 10.0F, .layer = kShipLayer, .mask = kWallLayer};
}

/// @brief An App with the PhysicsPlugin in a run mode, ticked by hand.
struct PhysicsApp {
    rtype::ecs::App app;  ///< The App under test.

    explicit PhysicsApp(RunMode mode) : app{mode} {
        app.addPlugin(PhysicsPlugin{smallSettings()});
        app.getWorld().getResources().insert(rtype::ecs::Time{.fixedDelta = kFixedDelta});
    }

    /// @brief Runs fixed ticks.
    void tick(int count = 1) {
        for (int index = 0; index < count; ++index) {
            app.getSchedules().run(rtype::ecs::Phase::kFixedUpdate);
        }
    }

    [[nodiscard]] const PhysicsWorld& world() const { return app.getWorld().getResources().require<PhysicsWorld>(); }
    [[nodiscard]] rtype::ecs::Components& components() { return app.getWorld().getComponents(); }

    /// @return A new entity with a Transform and the Collider, and a RigidBody when given.
    Entity spawn(glm::vec2 position, const Collider& collider, std::optional<RigidBody> body = std::nullopt) {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = position});
        components().add(entity, collider);
        if (body) {
            components().add(entity, *body);
        }
        return entity;
    }

    /// @return The entity's Transform position.
    [[nodiscard]] glm::vec2 position(Entity entity) { return components().get<Transform>(entity)->position; }
};

/// @brief What the server and every client know of a scene: a wall and a ship next to it.
struct Scene {
    PhysicsApp app;  ///< The App, in the side's mode.
    Entity wall;     ///< The wall, its left face at x = 90.
    Entity ship;     ///< The ship, at (50, 0), a kinematic body.

    explicit Scene(RunMode mode)
        : app{mode},
          wall{app.spawn({100.0F, 0.0F}, wallCollider())},
          ship{app.spawn({50.0F, 0.0F}, shipCollider(), RigidBody{.type = BodyType::kKinematic})} {
        app.tick();
    }
};

/// @return The ReplicationMode a component is registered with in the App.
ReplicationMode replicationOf(PhysicsApp& app, const char* name) {
    return app.components().getRegistry().findByName(name)->type.replication;
}
}  // namespace

TEST(PhysicsNetwork, ColliderAndRigidBodyAreReplicated) {
    PhysicsApp app{RunMode::kStandalone};
    EXPECT_EQ(replicationOf(app, "rtype.Collider"), ReplicationMode::kEveryChange);
    EXPECT_EQ(replicationOf(app, "rtype.RigidBody"), ReplicationMode::kEveryChange);
}

TEST(PhysicsNetwork, TheInternalComponentsAreNot) {
    PhysicsApp app{RunMode::kStandalone};
    EXPECT_EQ(replicationOf(app, "rtype.Velocity"), ReplicationMode::kNone);
    EXPECT_EQ(replicationOf(app, "rtype.PhysicsHandle"), ReplicationMode::kNone);
}

TEST(PhysicsNetwork, TheDebugPluginRegistersColliderTheSameWayInEitherOrder) {
    rtype::ecs::App debugFirst;
    debugFirst.addPlugin(PhysicsDebugPlugin{});
    EXPECT_NO_THROW(debugFirst.addPlugin(PhysicsPlugin{smallSettings()}));
    rtype::ecs::App physicsFirst;
    physicsFirst.addPlugin(PhysicsPlugin{smallSettings()});
    EXPECT_NO_THROW(physicsFirst.addPlugin(PhysicsDebugPlugin{}));
}

TEST(PhysicsNetwork, TheServerSimulates) {
    PhysicsApp server{RunMode::kServer};
    const Entity ball = server.spawn({0.0F, 0.0F}, Collider{}, RigidBody{});
    server.tick(30);
    EXPECT_EQ(server.world().getStepCount(), 30U);
    EXPECT_GT(server.position(ball).y, 10.0F);
}

TEST(PhysicsNetwork, StandaloneSimulatesAsBefore) {
    PhysicsApp standalone{RunMode::kStandalone};
    const Entity ball = standalone.spawn({0.0F, 0.0F}, Collider{}, RigidBody{});
    const Entity ship = standalone.spawn({0.0F, 300.0F}, shipCollider(), RigidBody{.type = BodyType::kKinematic});
    standalone.components().add(ship, Velocity{.linear = {60.0F, 0.0F}});
    standalone.tick(30);
    EXPECT_EQ(standalone.world().getStepCount(), 30U);
    EXPECT_GT(standalone.position(ball).y, 10.0F);
    // Moved by its Velocity, not teleported back to the Transform it had before each step.
    EXPECT_NEAR(standalone.position(ship).x, 30.0F, 1.0F);
}

TEST(PhysicsNetwork, AClientBuildsBodiesButNeverSteps) {
    PhysicsApp client{RunMode::kClient};
    const Entity ball = client.spawn({0.0F, 0.0F}, Collider{}, RigidBody{});
    client.tick(30);
    EXPECT_EQ(client.world().getStepCount(), 0U);
    EXPECT_EQ(client.world().getBodyCount(), 1U);
    EXPECT_TRUE(client.components().get<PhysicsHandle>(ball));
    // Gravity never moved it: only the server decides where it is.
    EXPECT_FLOAT_EQ(client.position(ball).y, 0.0F);
}

TEST(PhysicsNetwork, AClientBodyFollowsTheTransformReplicationWrites) {
    PhysicsApp client{RunMode::kClient};
    const Entity ball = client.spawn({0.0F, 0.0F}, Collider{}, RigidBody{});
    client.tick();
    client.components().getMut<Transform>(ball)->position = {300.0F, 40.0F};  // A snapshot arrived.
    client.tick();
    const std::optional<glm::vec2> body = client.world().getBodyPosition(ball);
    ASSERT_TRUE(body);
    EXPECT_NEAR(body->x, 300.0F, kTolerance);
    EXPECT_NEAR(body->y, 40.0F, kTolerance);
    const std::optional<PhysicsWorld::RayHit> hit = client.world().raycast({0.0F, 40.0F}, {1.0F, 0.0F}, 1000.0F);
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, ball);
}

TEST(PhysicsNetwork, AClientBodyGoesWhenItsColliderDoes) {
    PhysicsApp client{RunMode::kClient};
    const Entity ball = client.spawn({0.0F, 0.0F}, Collider{});
    client.tick();
    client.components().remove<Collider>(ball);
    client.tick();
    EXPECT_EQ(client.world().getBodyCount(), 0U);
}

TEST(MoveAndSlide, MovesTheWholeDeltaWithoutObstacles) {
    const Scene scene{RunMode::kStandalone};
    const glm::vec2 end = moveAndSlide(scene.app.world(), scene.ship, shipCollider(), {50.0F, 0.0F}, {-30.0F, 20.0F});
    EXPECT_NEAR(end.x, 20.0F, kTolerance);
    EXPECT_NEAR(end.y, 20.0F, kTolerance);
}

TEST(MoveAndSlide, StopsASkinAwayFromAWall) {
    const Scene scene{RunMode::kStandalone};
    const glm::vec2 end = moveAndSlide(scene.app.world(), scene.ship, shipCollider(), {50.0F, 0.0F}, {100.0F, 0.0F},
                                       SlideSettings{.skin = 1.0F});
    EXPECT_NEAR(end.x, 79.0F, kTolerance);  // The face at 90, minus the radius, minus the skin.
    EXPECT_NEAR(end.y, 0.0F, kTolerance);
}

TEST(MoveAndSlide, SlidesAlongAWall) {
    const Scene scene{RunMode::kStandalone};
    const glm::vec2 end = moveAndSlide(scene.app.world(), scene.ship, shipCollider(), {50.0F, 0.0F}, {100.0F, 60.0F});
    EXPECT_NEAR(end.x, 79.5F, kTolerance);
    EXPECT_NEAR(end.y, 60.0F, kTolerance);  // The part along the wall is kept whole.
}

TEST(MoveAndSlide, StopsWithoutSlidesLeft) {
    const Scene scene{RunMode::kStandalone};
    const glm::vec2 end = moveAndSlide(scene.app.world(), scene.ship, shipCollider(), {50.0F, 0.0F}, {100.0F, 60.0F},
                                       SlideSettings{.maxSlides = 0});
    EXPECT_NEAR(end.x, 79.5F, kTolerance);
    EXPECT_LT(end.y, 60.0F);
}

TEST(MoveAndSlide, IsNeverBlockedByItsOwnBody) {
    const Scene scene{RunMode::kStandalone};
    const QueryFilter blind{};  // Sees the ship's own body, which it starts inside.
    const glm::vec2 stuck = moveAndSlide(scene.app.world(), shipCollider(), {50.0F, 0.0F}, {-20.0F, 0.0F}, blind);
    EXPECT_NEAR(stuck.x, 50.0F, kTolerance);
    const glm::vec2 free = moveAndSlide(scene.app.world(), scene.ship, shipCollider(), {50.0F, 0.0F}, {-20.0F, 0.0F});
    EXPECT_NEAR(free.x, 30.0F, kTolerance);
}

TEST(MoveAndSlide, IgnoresTheLayersOutsideTheMask) {
    const Scene scene{RunMode::kStandalone};
    Collider ghost = shipCollider();
    ghost.mask = kShipLayer;
    const glm::vec2 end = moveAndSlide(scene.app.world(), scene.ship, ghost, {50.0F, 0.0F}, {100.0F, 0.0F});
    EXPECT_NEAR(end.x, 150.0F, kTolerance);
}

TEST(MoveAndSlide, DoesNothingForANullDelta) {
    const Scene scene{RunMode::kStandalone};
    const glm::vec2 end = moveAndSlide(scene.app.world(), scene.ship, shipCollider(), {50.0F, 0.0F}, {0.0F, 0.0F});
    EXPECT_FLOAT_EQ(end.x, 50.0F);
    EXPECT_FLOAT_EQ(end.y, 0.0F);
}

TEST(MoveAndSlide, RefusesANegativeSkin) {
    const Scene scene{RunMode::kStandalone};
    EXPECT_THROW(static_cast<void>(moveAndSlide(scene.app.world(), scene.ship, shipCollider(), {50.0F, 0.0F},
                                                {10.0F, 0.0F}, SlideSettings{.skin = -1.0F})),
                 PhysicsException);
}

TEST(MoveAndSlide, PredictionAndServerAgreeOnAShipSlidingAlongAWall) {
    Scene server{RunMode::kServer};
    Scene client{RunMode::kClient};
    EXPECT_EQ(client.app.world().getStepCount(), 0U);
    glm::vec2 onServer{50.0F, 0.0F};
    glm::vec2 predicted{50.0F, 0.0F};
    // The same inputs, replayed on both sides: into the wall and along it.
    for (int input = 0; input < 20; ++input) {
        const glm::vec2 delta{5.0F, 3.0F};
        onServer = moveAndSlide(server.app.world(), server.ship, shipCollider(), onServer, delta);
        predicted = moveAndSlide(client.app.world(), client.ship, shipCollider(), predicted, delta);
        ASSERT_NEAR(predicted.x, onServer.x, 1e-3F) << "input " << input;
        ASSERT_NEAR(predicted.y, onServer.y, 1e-3F) << "input " << input;
    }
    EXPECT_NEAR(onServer.x, 79.5F, kTolerance);
    EXPECT_NEAR(onServer.y, 60.0F, kTolerance);
}
