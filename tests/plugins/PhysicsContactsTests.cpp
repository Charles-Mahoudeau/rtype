/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsContactsTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/CollisionEvents.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

using rtype::ecs::Entity;
using rtype::plugins::physics::Collider;
using rtype::plugins::physics::CollisionEnded;
using rtype::plugins::physics::CollisionStarted;
using rtype::plugins::physics::PhysicsPlugin;
using rtype::plugins::physics::PhysicsSettings;
using rtype::plugins::physics::RigidBody;
using rtype::plugins::physics::ShapeType;
using rtype::plugins::physics::TriggerEntered;
using rtype::plugins::physics::TriggerExited;
using rtype::plugins::physics::Velocity;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;
constexpr int kOneSecond = 60;
constexpr float kTolerance = 1e-3F;

/// @brief Every contact event the physics plugin sent, in order.
struct Received {
    std::vector<CollisionStarted> started;  ///< CollisionStarted events.
    std::vector<CollisionEnded> ended;      ///< CollisionEnded events.
    std::vector<TriggerEntered> entered;    ///< TriggerEntered events.
    std::vector<TriggerExited> exited;      ///< TriggerExited events.
};
RTYPE_ECS_RESOURCE(Received, "test.Received");

/// @brief FixedUpdate, after the physics: records the contact events.
void record(rtype::ecs::ResMut<Received> received, rtype::ecs::EventReader<CollisionStarted> started,
            rtype::ecs::EventReader<CollisionEnded> ended, rtype::ecs::EventReader<TriggerEntered> entered,
            rtype::ecs::EventReader<TriggerExited> exited) {
    for (const CollisionStarted& event : started) {
        received->started.push_back(event);
    }
    for (const CollisionEnded& event : ended) {
        received->ended.push_back(event);
    }
    for (const TriggerEntered& event : entered) {
        received->entered.push_back(event);
    }
    for (const TriggerExited& event : exited) {
        received->exited.push_back(event);
    }
}

/// @return Whether the pair is the two entities, in either order.
bool isPair(Entity first, Entity second, Entity a, Entity b) {
    return (first == a && second == b) || (first == b && second == a);
}

/// @brief An App with the PhysicsPlugin, 100 units per meter, ticked by hand, recording the contact events.
struct PhysicsApp {
    rtype::ecs::App app;  ///< The App under test.

    explicit PhysicsApp(glm::vec2 gravity = {0.0F, 0.0F}) {
        app.addPlugin(PhysicsPlugin{PhysicsSettings{.gravity = gravity,
                                                    .unitsPerMeter = 100.0F,
                                                    .maxBodies = 16,
                                                    .maxBodyPairs = 16,
                                                    .maxContactConstraints = 16,
                                                    .tempAllocatorBytes = 1024 * 1024}});
        app.getWorld().getResources().insert(rtype::ecs::Time{.fixedDelta = kFixedDelta});
        app.insertResource(Received{});
        app.addSystem(rtype::ecs::Phase::kFixedUpdate, "test.Record", &record)
            .after(std::string{PhysicsPlugin::kEventsSystem});
    }

    /// @brief Runs the given number of fixed ticks.
    void tick(int count = 1) {
        for (int index = 0; index < count; ++index) {
            app.getSchedules().run(rtype::ecs::Phase::kFixedUpdate);
        }
    }

    [[nodiscard]] rtype::ecs::Components& components() { return app.getWorld().getComponents(); }
    [[nodiscard]] const Received& received() const { return app.getWorld().getResources().require<Received>(); }

    /// @return A circle at the position; moving at the velocity when given, static otherwise.
    Entity spawnBall(glm::vec2 position, std::optional<glm::vec2> velocity, Collider collider = {}) {
        collider.shape = ShapeType::kCircle;
        collider.radius = 16.0F;
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = position});
        components().add(entity, collider);
        if (velocity) {
            components().add(entity, RigidBody{.linearDamping = 0.0F, .angularDamping = 0.0F});
            components().add(entity, Velocity{.linear = *velocity});
        }
        return entity;
    }

    /// @return A static floor whose top is at y = 100.
    Entity spawnFloor() {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = {0.0F, 116.0F}});
        components().add(entity, Collider{.shape = ShapeType::kBox, .size = {1000.0F, 32.0F}});
        return entity;
    }
};
}  // namespace

TEST(PhysicsContacts, TwoBodiesThatMeetSendOneStartThenOneEnd) {
    PhysicsApp fixture;
    const Collider bouncy{.restitution = 1.0F};
    const Entity left = fixture.spawnBall({-100.0F, 0.0F}, glm::vec2{300.0F, 0.0F}, bouncy);
    const Entity right = fixture.spawnBall({100.0F, 0.0F}, glm::vec2{-300.0F, 0.0F}, bouncy);
    fixture.tick(kOneSecond);  // They meet, bounce back and fly apart.

    ASSERT_EQ(fixture.received().started.size(), 1U);
    const CollisionStarted& started = fixture.received().started.front();
    EXPECT_TRUE(isPair(started.first, started.second, left, right));
    EXPECT_NEAR(started.point.y, 0.0F, 1.0F);
    EXPECT_LT(std::abs(started.point.x), 20.0F);  // They meet around x = 0.
    // The normal is a unit vector along X, from first to second.
    const float towardSecond = started.first == left ? 1.0F : -1.0F;
    EXPECT_NEAR(started.normal.x, towardSecond, kTolerance);
    EXPECT_NEAR(started.normal.y, 0.0F, kTolerance);

    ASSERT_EQ(fixture.received().ended.size(), 1U);
    EXPECT_TRUE(isPair(fixture.received().ended.front().first, fixture.received().ended.front().second, left, right));
    EXPECT_TRUE(fixture.received().entered.empty());
}

TEST(PhysicsContacts, ALastingContactSendsOneStartAndNoEnd) {
    PhysicsApp fixture{{0.0F, 980.0F}};
    const Entity floor = fixture.spawnFloor();
    const Entity ball = fixture.spawnBall({0.0F, 0.0F}, glm::vec2{0.0F, 0.0F});
    fixture.tick(2 * kOneSecond);  // Falls, lands and rests on the floor.

    ASSERT_EQ(fixture.received().started.size(), 1U);
    EXPECT_TRUE(
        isPair(fixture.received().started.front().first, fixture.received().started.front().second, floor, ball));
    EXPECT_TRUE(fixture.received().ended.empty());
}

TEST(PhysicsContacts, ASleepingBodyStaysInContactUntilItReallyLeaves) {
    PhysicsApp fixture{{0.0F, 980.0F}};
    const Entity floor = fixture.spawnFloor();
    const Entity ball = fixture.spawnBall({0.0F, 0.0F}, glm::vec2{0.0F, 0.0F});
    fixture.tick(3 * kOneSecond);  // Lands, then falls asleep: Jolt removes its contacts, but it still touches.
    ASSERT_EQ(fixture.received().started.size(), 1U);
    ASSERT_TRUE(fixture.received().ended.empty());

    fixture.app.getWorld().getResources().requireMut<rtype::plugins::physics::PhysicsWorld>()->applyImpulse(
        ball, {0.0F, -800.0F});  // Kicked upward: wakes up, then leaves the floor.
    fixture.tick(10);
    EXPECT_EQ(fixture.received().started.size(), 1U);  // Waking up is not a new contact.
    ASSERT_EQ(fixture.received().ended.size(), 1U);
    EXPECT_TRUE(isPair(fixture.received().ended.front().first, fixture.received().ended.front().second, floor, ball));
}

TEST(PhysicsContacts, ASensorSendsEnteredThenExitedWithItselfAsTheTrigger) {
    PhysicsApp fixture;
    const Entity zone = fixture.spawnBall({0.0F, 0.0F}, std::nullopt, Collider{.sensor = true});
    const Entity ball = fixture.spawnBall({-100.0F, 0.0F}, glm::vec2{300.0F, 0.0F});
    fixture.tick(kOneSecond);  // Flies through the zone.

    ASSERT_EQ(fixture.received().entered.size(), 1U);
    EXPECT_EQ(fixture.received().entered.front().trigger, zone);
    EXPECT_EQ(fixture.received().entered.front().other, ball);
    ASSERT_EQ(fixture.received().exited.size(), 1U);
    EXPECT_EQ(fixture.received().exited.front().trigger, zone);
    EXPECT_EQ(fixture.received().exited.front().other, ball);
    EXPECT_TRUE(fixture.received().started.empty());  // A sensor never collides.
    EXPECT_GT(fixture.components().get<Transform>(ball)->position.x, 150.0F);
}

TEST(PhysicsContacts, DespawningDuringAContactSendsNoEnd) {
    PhysicsApp fixture{{0.0F, 980.0F}};
    fixture.spawnFloor();
    const Entity ball = fixture.spawnBall({0.0F, 0.0F}, glm::vec2{0.0F, 0.0F});
    fixture.tick(kOneSecond);
    ASSERT_EQ(fixture.received().started.size(), 1U);

    ASSERT_TRUE(fixture.app.getWorld().despawn(ball));
    fixture.tick(10);
    EXPECT_TRUE(fixture.received().ended.empty());
}

TEST(PhysicsContacts, RebuildingABodyDuringAContactSendsNoEnd) {
    PhysicsApp fixture{{0.0F, 980.0F}};
    fixture.spawnFloor();
    const Entity ball = fixture.spawnBall({0.0F, 0.0F}, glm::vec2{0.0F, 0.0F});
    fixture.tick(kOneSecond);
    ASSERT_EQ(fixture.received().started.size(), 1U);

    Collider collider = *fixture.components().get<Collider>(ball);
    collider.friction = 0.8F;
    fixture.components().add(ball, collider);  // The body is rebuilt: its contacts start over.
    fixture.tick(10);
    EXPECT_TRUE(fixture.received().ended.empty());
    EXPECT_EQ(fixture.received().started.size(), 2U);
}

TEST(PhysicsContacts, FilteredPairsSendNothing) {
    PhysicsApp fixture;
    fixture.spawnBall({-100.0F, 0.0F}, glm::vec2{300.0F, 0.0F}, Collider{.layer = 1U << 0, .mask = 1U << 1});
    fixture.spawnBall({100.0F, 0.0F}, glm::vec2{-300.0F, 0.0F}, Collider{.layer = 1U << 2, .mask = 1U << 1});
    fixture.tick(kOneSecond);  // Their masks ignore each other: they pass through.
    EXPECT_TRUE(fixture.received().started.empty());
    EXPECT_TRUE(fixture.received().ended.empty());
}

TEST(PhysicsContacts, ThePluginRegistersTheEvents) {
    PhysicsApp fixture;
    EXPECT_TRUE(fixture.app.getWorld().getEvents().find("rtype.CollisionStarted"));
    EXPECT_TRUE(fixture.app.getWorld().getEvents().find("rtype.CollisionEnded"));
    EXPECT_TRUE(fixture.app.getWorld().getEvents().find("rtype.TriggerEntered"));
    EXPECT_TRUE(fixture.app.getWorld().getEvents().find("rtype.TriggerExited"));
}
