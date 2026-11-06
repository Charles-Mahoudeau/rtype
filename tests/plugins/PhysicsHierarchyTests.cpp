/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsHierarchyTests
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
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/Time.hpp"

using rtype::ecs::Entity;
using rtype::ecs::RunMode;
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
constexpr float kTolerance = 0.01F;

/// @brief An App with the PhysicsPlugin and no gravity, ticked by hand.
struct HierarchyApp {
    rtype::ecs::App app;  ///< The App under test.

    explicit HierarchyApp(RunMode mode = RunMode::kStandalone) : app{mode} {
        app.addPlugin(PhysicsPlugin{PhysicsSettings{.gravity = {0.0F, 0.0F},
                                                    .unitsPerMeter = 100.0F,
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
    [[nodiscard]] const PhysicsWorld& physics() const { return app.getWorld().getResources().require<PhysicsWorld>(); }

    /// @return A new entity at @p position, with neither Collider nor RigidBody.
    Entity spawn(glm::vec2 position) {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = position});
        return entity;
    }

    /// @return A new child of @p parent at @p position, with a circle Collider and a body of @p type.
    Entity spawnBody(Entity parent, glm::vec2 position, BodyType type) {
        const Entity entity = spawn(position);
        components().add(entity, Collider{.shape = ShapeType::kCircle, .radius = 8.0F});
        components().add(entity, RigidBody{.type = type});
        EXPECT_TRUE(app.getWorld().setParent(entity, parent));
        return entity;
    }

    /// @brief Moves @p entity, as gameplay does.
    void move(Entity entity, glm::vec2 position) { components().getMut<Transform>(entity)->position = position; }

    /// @brief Expects the body of @p entity at @p position.
    void expectBodyAt(Entity entity, glm::vec2 position) const {
        const std::optional<glm::vec2> body = physics().getBodyPosition(entity);
        ASSERT_TRUE(body);
        EXPECT_NEAR(body->x, position.x, kTolerance);
        EXPECT_NEAR(body->y, position.y, kTolerance);
    }
};
}  // namespace

TEST(PhysicsHierarchy, CreatesAChildBodyWhereItsParentPutsIt) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 50.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 5.0F}, BodyType::kStatic);
    fixture.tick();
    fixture.expectBodyAt(child, {110.0F, 55.0F});
}

TEST(PhysicsHierarchy, GoesUpAWholeChain) {
    HierarchyApp fixture;
    const Entity root = fixture.spawn({100.0F, 0.0F});
    const Entity middle = fixture.spawn({10.0F, 0.0F});
    EXPECT_TRUE(fixture.app.getWorld().setParent(middle, root));
    const Entity child = fixture.spawnBody(middle, {1.0F, 0.0F}, BodyType::kStatic);
    fixture.tick();
    fixture.expectBodyAt(child, {111.0F, 0.0F});
}

TEST(PhysicsHierarchy, AStaticChildFollowsItsParent) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kStatic);
    fixture.tick();
    fixture.move(parent, {200.0F, 40.0F});
    fixture.tick();
    fixture.expectBodyAt(child, {210.0F, 40.0F});
}

TEST(PhysicsHierarchy, AKinematicChildFollowsItsParent) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kKinematic);
    fixture.tick();
    fixture.move(parent, {130.0F, 0.0F});
    fixture.tick(2);
    fixture.expectBodyAt(child, {140.0F, 0.0F});
}

TEST(PhysicsHierarchy, LeavesTheTransformOfAKinematicChildRelative) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kKinematic);
    fixture.tick();
    fixture.move(parent, {130.0F, 0.0F});
    fixture.tick(3);
    EXPECT_EQ(fixture.components().get<Transform>(child)->position, (glm::vec2{10.0F, 0.0F}));
}

TEST(PhysicsHierarchy, AChildMovedByGameplayStaysRelativeToItsParent) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kStatic);
    fixture.tick();
    fixture.move(child, {20.0F, 5.0F});
    fixture.tick();
    fixture.expectBodyAt(child, {120.0F, 5.0F});
}

TEST(PhysicsHierarchy, ADetachedChildGoesBackToItsTransform) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kStatic);
    fixture.tick();
    EXPECT_TRUE(fixture.app.getWorld().removeParent(child));
    fixture.tick();
    fixture.expectBodyAt(child, {10.0F, 0.0F});
}

TEST(PhysicsHierarchy, AClientBodyFollowsItsParent) {
    HierarchyApp fixture{RunMode::kClient};
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kKinematic);
    fixture.tick();
    fixture.move(parent, {300.0F, 0.0F});
    fixture.tick();
    fixture.expectBodyAt(child, {310.0F, 0.0F});
}

TEST(PhysicsHierarchy, ADynamicChildDoesNotFollowItsParent) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kDynamic);
    fixture.tick();
    fixture.expectBodyAt(child, {110.0F, 0.0F});
    fixture.move(parent, {300.0F, 0.0F});
    fixture.tick(3);
    fixture.expectBodyAt(child, {110.0F, 0.0F});
}

TEST(PhysicsHierarchy, ADynamicChildKeepsItsPlaceInTheWorldThroughItsTransform) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kDynamic);
    fixture.tick();
    fixture.move(parent, {300.0F, 0.0F});
    fixture.tick();
    const Transform* local = fixture.components().get<Transform>(child);
    EXPECT_NEAR(local->position.x, -190.0F, kTolerance);
}

TEST(PhysicsHierarchy, ADynamicChildMovesWithItsVelocityAndWritesItRelative) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kDynamic);
    fixture.components().add(child, Velocity{.linear = {60.0F, 0.0F}});
    fixture.tick(60);
    const float body = fixture.physics().getBodyPosition(child)->x;
    EXPECT_GT(body, 160.0F);  // a little under 170: bodies have a default damping
    EXPECT_NEAR(fixture.components().get<Transform>(child)->position.x, body - 100.0F, kTolerance);
}

TEST(PhysicsHierarchy, GameplayTeleportsADynamicChild) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kDynamic);
    fixture.tick(2);
    fixture.move(child, {50.0F, 20.0F});
    fixture.tick();
    fixture.expectBodyAt(child, {150.0F, 20.0F});
}

TEST(PhysicsHierarchy, ARestingDynamicChildIsNotTeleportedUnderATurnedParent) {
    HierarchyApp fixture;
    const Entity parent = fixture.app.getWorld().spawn();
    fixture.components().add(parent, Transform{.position = {33.3F, 71.7F}, .rotation = 0.7F, .scale = {1.3F, 1.3F}});
    const Entity child = fixture.spawnBody(parent, {12.1F, -7.3F}, BodyType::kDynamic);
    fixture.tick();
    const glm::vec2 first = *fixture.physics().getBodyPosition(child);
    fixture.tick(30);
    EXPECT_EQ(*fixture.physics().getBodyPosition(child), first);
}

TEST(PhysicsHierarchy, ADynamicChildOfADynamicParentIsWrittenRelativeToItsNewPose) {
    HierarchyApp fixture;
    const Entity root = fixture.spawn({0.0F, 0.0F});
    const Entity parent = fixture.spawnBody(root, {100.0F, 0.0F}, BodyType::kDynamic);
    const Entity child = fixture.spawnBody(parent, {200.0F, 0.0F}, BodyType::kDynamic);  // far enough not to touch
    fixture.components().add(parent, Velocity{.linear = {60.0F, 0.0F}});
    fixture.tick(60);
    const float parentBody = fixture.physics().getBodyPosition(parent)->x;
    EXPECT_GT(parentBody, 150.0F);
    fixture.expectBodyAt(child, {300.0F, 0.0F});
    EXPECT_NEAR(fixture.components().get<Transform>(child)->position.x, 300.0F - parentBody, kTolerance);
}

TEST(PhysicsHierarchy, AKinematicChildWithAVelocityMovesAndFollowsItsParent) {
    HierarchyApp fixture;
    const Entity parent = fixture.spawn({100.0F, 0.0F});
    const Entity child = fixture.spawnBody(parent, {10.0F, 0.0F}, BodyType::kKinematic);
    fixture.components().add(child, Velocity{.linear = {60.0F, 0.0F}});
    fixture.tick(60);
    const float before = fixture.physics().getBodyPosition(child)->x;
    EXPECT_GT(before, 160.0F);
    EXPECT_NEAR(fixture.components().get<Transform>(child)->position.x, before - 100.0F, kTolerance);
    fixture.move(parent, {200.0F, 0.0F});
    fixture.tick(2);
    EXPECT_NEAR(fixture.physics().getBodyPosition(child)->x, before + 100.0F, 5.0F);
}
