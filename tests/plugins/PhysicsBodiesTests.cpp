/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsBodiesTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <random>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/time/Time.hpp"

using rtype::ecs::Entity;
using rtype::plugins::physics::BodyType;
using rtype::plugins::physics::Collider;
using rtype::plugins::physics::PhysicsHandle;
using rtype::plugins::physics::PhysicsPlugin;
using rtype::plugins::physics::PhysicsSettings;
using rtype::plugins::physics::PhysicsWorld;
using rtype::plugins::physics::RigidBody;
using rtype::plugins::physics::ShapeType;
using rtype::plugins::physics::exceptions::PhysicsException;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;
constexpr std::uint32_t kMaxBodies = 64;

/// @brief Small capacities, so tests build worlds quickly.
PhysicsSettings smallSettings() {
    return PhysicsSettings{.maxBodies = kMaxBodies,
                           .maxBodyPairs = kMaxBodies,
                           .maxContactConstraints = kMaxBodies,
                           .tempAllocatorBytes = 1024 * 1024};
}

/// @brief An App with the PhysicsPlugin, ticked by hand.
struct PhysicsApp {
    rtype::ecs::App app;  ///< The App under test.

    explicit PhysicsApp(PhysicsSettings settings = smallSettings()) {
        app.addPlugin(PhysicsPlugin{settings});
        app.getWorld().getResources().insert(rtype::ecs::Time{.fixedDelta = kFixedDelta});
    }

    /// @brief Runs one fixed tick: the body sync, then the step.
    void tick() { app.getSchedules().run(rtype::ecs::Phase::kFixedUpdate); }

    [[nodiscard]] const PhysicsWorld& world() const { return app.getWorld().getResources().require<PhysicsWorld>(); }
    [[nodiscard]] rtype::ecs::Components& components() { return app.getWorld().getComponents(); }

    /// @return A new entity with a Transform and a Collider, and a RigidBody when given.
    Entity spawn(glm::vec2 position = {0.0F, 0.0F}, std::optional<RigidBody> body = std::nullopt) {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = position});
        components().add(entity, Collider{});
        if (body) {
            components().add(entity, *body);
        }
        return entity;
    }

    /// @return The entity's PhysicsHandle component, or nothing.
    [[nodiscard]] std::optional<PhysicsHandle> handleOf(Entity entity) {
        const PhysicsHandle* const handle = components().get<PhysicsHandle>(entity);
        return handle != nullptr ? std::optional{*handle} : std::nullopt;
    }
};
}  // namespace

TEST(PhysicsBodies, ACollidingEntityGetsABodyAndAHandle) {
    PhysicsApp fixture;
    const Entity wall = fixture.spawn({100.0F, 50.0F});
    fixture.tick();

    EXPECT_EQ(fixture.world().getBodyCount(), 1U);
    const std::optional<PhysicsHandle> handle = fixture.handleOf(wall);
    ASSERT_TRUE(handle);
    EXPECT_EQ(fixture.world().findBody(wall), handle);
    EXPECT_EQ(fixture.world().findEntity(*handle), wall);
}

TEST(PhysicsBodies, TheBodyStartsAtTheTransform) {
    PhysicsApp fixture;
    const Entity wall = fixture.spawn({100.0F, 50.0F});
    fixture.tick();

    const std::optional<glm::vec2> position = fixture.world().getBodyPosition(wall);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->x, 100.0F, 1e-3F);
    EXPECT_NEAR(position->y, 50.0F, 1e-3F);
}

TEST(PhysicsBodies, AnEntityWithoutTransformGetsNoBody) {
    PhysicsApp fixture;
    const Entity entity = fixture.app.getWorld().spawn();
    fixture.components().add(entity, Collider{});
    fixture.tick();
    EXPECT_EQ(fixture.world().getBodyCount(), 0U);
    EXPECT_FALSE(fixture.handleOf(entity));
}

TEST(PhysicsBodies, DespawningDestroysTheBody) {
    PhysicsApp fixture;
    const Entity ship = fixture.spawn({}, RigidBody{});
    fixture.tick();
    ASSERT_EQ(fixture.world().getBodyCount(), 1U);

    ASSERT_TRUE(fixture.app.getWorld().despawn(ship));
    fixture.tick();
    EXPECT_EQ(fixture.world().getBodyCount(), 0U);
    EXPECT_FALSE(fixture.world().findBody(ship));
}

TEST(PhysicsBodies, RemovingTheColliderDestroysTheBodyAndTheHandle) {
    PhysicsApp fixture;
    const Entity ship = fixture.spawn();
    fixture.tick();
    ASSERT_TRUE(fixture.handleOf(ship));

    ASSERT_TRUE(fixture.components().remove<Collider>(ship));
    fixture.tick();
    EXPECT_EQ(fixture.world().getBodyCount(), 0U);
    EXPECT_FALSE(fixture.handleOf(ship));
}

TEST(PhysicsBodies, AddingOrRemovingTheRigidBodyRebuildsTheBody) {
    PhysicsApp fixture;
    const Entity entity = fixture.spawn();
    fixture.tick();
    const std::optional<PhysicsHandle> asStatic = fixture.handleOf(entity);

    fixture.components().add(entity, RigidBody{});
    fixture.tick();
    const std::optional<PhysicsHandle> asDynamic = fixture.handleOf(entity);
    EXPECT_NE(asDynamic, asStatic);
    EXPECT_EQ(fixture.world().getBodyCount(), 1U);

    ASSERT_TRUE(fixture.components().remove<RigidBody>(entity));
    fixture.tick();
    EXPECT_NE(fixture.handleOf(entity), asDynamic);
    EXPECT_EQ(fixture.world().getBodyCount(), 1U);
}

TEST(PhysicsBodies, ChangingTheColliderRebuildsTheBody) {
    PhysicsApp fixture;
    const Entity entity = fixture.spawn();
    fixture.tick();
    const std::optional<PhysicsHandle> before = fixture.handleOf(entity);

    fixture.components().add(entity, Collider{.shape = ShapeType::kCircle, .radius = 4.0F});
    fixture.tick();
    EXPECT_NE(fixture.handleOf(entity), before);
    EXPECT_EQ(fixture.world().getBodyCount(), 1U);
}

TEST(PhysicsBodies, AnUnchangedEntityKeepsItsBody) {
    PhysicsApp fixture;
    const Entity entity = fixture.spawn();
    fixture.tick();
    const std::optional<PhysicsHandle> before = fixture.handleOf(entity);
    fixture.tick();
    fixture.tick();
    EXPECT_EQ(fixture.handleOf(entity), before);
}

TEST(PhysicsBodies, ADynamicBodyFallsUnderGravity) {
    PhysicsSettings settings = smallSettings();
    settings.gravity = {0.0F, 980.0F};
    settings.unitsPerMeter = 100.0F;
    PhysicsApp fixture{settings};
    const Entity ball = fixture.spawn({0.0F, 0.0F}, RigidBody{.linearDamping = 0.0F});
    fixture.tick();  // Creates the body, then steps once.
    for (int tick = 0; tick < 59; ++tick) {
        fixture.tick();
    }

    // About one second of free fall: y = g t^2 / 2, so close to 490 units down (Y is down), and no sideways drift.
    const std::optional<glm::vec2> position = fixture.world().getBodyPosition(ball);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->x, 0.0F, 1e-3F);
    EXPECT_NEAR(position->y, 490.0F, 20.0F);
}

TEST(PhysicsBodies, AStaticBodyDoesNotFall) {
    PhysicsSettings settings = smallSettings();
    settings.gravity = {0.0F, 980.0F};
    PhysicsApp fixture{settings};
    const Entity wall = fixture.spawn({10.0F, 20.0F});
    for (int tick = 0; tick < 30; ++tick) {
        fixture.tick();
    }
    const std::optional<glm::vec2> position = fixture.world().getBodyPosition(wall);
    ASSERT_TRUE(position);
    EXPECT_NEAR(position->y, 20.0F, 1e-3F);
}

TEST(PhysicsBodies, TheBodyCountFollowsAnySequenceOfChanges) {
    PhysicsApp fixture;
    std::mt19937 random{42};
    std::vector<Entity> entities;
    for (int round = 0; round < 200; ++round) {
        const std::uint32_t action = random() % 4;
        if (action == 0 || entities.empty()) {
            entities.push_back(fixture.spawn({static_cast<float>(round), 0.0F}));
        } else {
            const std::size_t index = random() % entities.size();
            const Entity entity = entities.at(index);
            if (action == 1) {
                ASSERT_TRUE(fixture.app.getWorld().despawn(entity));
                entities.erase(entities.begin() + static_cast<std::ptrdiff_t>(index));
            } else if (action == 2) {
                if (fixture.components().get<RigidBody>(entity) != nullptr) {
                    static_cast<void>(fixture.components().remove<RigidBody>(entity));
                } else {
                    fixture.components().add(entity, RigidBody{.type = BodyType::kKinematic});
                }
            } else if (fixture.components().get<Collider>(entity) != nullptr) {
                static_cast<void>(fixture.components().remove<Collider>(entity));
            } else {
                fixture.components().add(entity, Collider{});
            }
        }
        fixture.tick();

        std::size_t colliders = 0;
        for (const Entity entity : entities) {
            if (fixture.components().get<Collider>(entity) != nullptr) {
                ++colliders;
            }
        }
        ASSERT_EQ(fixture.world().getBodyCount(), colliders) << "round " << round;
    }
}

TEST(PhysicsWorldBodies, RejectsInvalidComponents) {
    PhysicsWorld world{smallSettings()};
    const Entity entity{1, 1};
    EXPECT_THROW(static_cast<void>(world.createBody(entity, Collider{.shape = ShapeType::kCircle, .radius = 0.0F},
                                                    nullptr, Transform{})),
                 PhysicsException);
    EXPECT_THROW(static_cast<void>(world.createBody(entity, Collider{.size = {0.0F, 8.0F}}, nullptr, Transform{})),
                 PhysicsException);
    const RigidBody massless{.mass = 0.0F};
    EXPECT_THROW(static_cast<void>(world.createBody(entity, Collider{}, &massless, Transform{})), PhysicsException);
    EXPECT_EQ(world.getBodyCount(), 0U);
}

TEST(PhysicsWorldBodies, ThrowsWhenFull) {
    PhysicsSettings settings = smallSettings();
    settings.maxBodies = 2;
    PhysicsWorld world{settings};
    static_cast<void>(world.createBody(Entity{1, 1}, Collider{}, nullptr, Transform{}));
    static_cast<void>(world.createBody(Entity{2, 1}, Collider{}, nullptr, Transform{}));
    EXPECT_THROW(static_cast<void>(world.createBody(Entity{3, 1}, Collider{}, nullptr, Transform{})), PhysicsException);
}

TEST(PhysicsWorldBodies, EveryShapeBuilds) {
    PhysicsWorld world{smallSettings()};
    const std::array colliders{
        Collider{.shape = ShapeType::kBox, .size = {4.0F, 64.0F}},
        Collider{.shape = ShapeType::kCircle, .radius = 8.0F},
        Collider{.shape = ShapeType::kCapsule, .radius = 8.0F, .halfHeight = 16.0F},
        Collider{.shape = ShapeType::kCircle, .radius = 8.0F, .offset = {10.0F, -5.0F}},
    };
    std::uint32_t index = 1;
    for (const Collider& collider : colliders) {
        static_cast<void>(world.createBody(Entity{index++, 1}, collider, nullptr, Transform{}));
    }
    EXPECT_EQ(world.getBodyCount(), colliders.size());
}

TEST(PhysicsWorldBodies, DestroyingAMissingBodyReturnsFalse) {
    PhysicsWorld world{smallSettings()};
    EXPECT_FALSE(world.destroyBody(Entity{1, 1}));
    EXPECT_FALSE(world.findEntity(PhysicsHandle{}));
}
