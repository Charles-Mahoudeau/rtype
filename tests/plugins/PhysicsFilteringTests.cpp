/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsFilteringTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/time/Time.hpp"

using rtype::ecs::Entity;
using rtype::plugins::physics::Collider;
using rtype::plugins::physics::kAllLayers;
using rtype::plugins::physics::PhysicsPlugin;
using rtype::plugins::physics::PhysicsSettings;
using rtype::plugins::physics::PhysicsWorld;
using rtype::plugins::physics::RigidBody;
using rtype::plugins::physics::ShapeType;
using rtype::plugins::physics::Velocity;
using rtype::plugins::physics::exceptions::PhysicsException;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;
constexpr float kSpeed = 300.0F;
constexpr int kOneSecond = 60;

constexpr std::uint32_t kPlayer = 1U << 0;
constexpr std::uint32_t kPlayerBullet = 1U << 1;
constexpr std::uint32_t kEnemy = 1U << 2;
constexpr std::uint32_t kWall = 1U << 3;

/// @brief A body that nothing slows down.
constexpr RigidBody kFreeBody{.linearDamping = 0.0F, .angularDamping = 0.0F, .fixedRotation = true};

/// @brief An App with the PhysicsPlugin, no gravity, 100 units per meter, ticked by hand.
struct PhysicsApp {
    rtype::ecs::App app;  ///< The App under test.

    PhysicsApp() {
        app.addPlugin(PhysicsPlugin{PhysicsSettings{.unitsPerMeter = 100.0F,
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
    [[nodiscard]] float xOf(Entity entity) { return components().get<Transform>(entity)->position.x; }

    /// @return A circle at the position, in the categories, colliding with the mask; moving at the speed if given,
    /// static otherwise.
    Entity spawn(glm::vec2 position, std::uint32_t layer, std::uint32_t mask, std::optional<float> speed) {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = position});
        components().add(entity, Collider{.shape = ShapeType::kCircle, .radius = 16.0F, .layer = layer, .mask = mask});
        if (speed) {
            components().add(entity, kFreeBody);
            components().add(entity, Velocity{.linear = {*speed, 0.0F}});
        }
        return entity;
    }

    /// @return A static wall across the X axis at x = 0, in the categories, colliding with the mask.
    Entity spawnWall(std::uint32_t layer, std::uint32_t mask) {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = {0.0F, 0.0F}});
        components().add(entity,
                         Collider{.shape = ShapeType::kBox, .size = {32.0F, 400.0F}, .layer = layer, .mask = mask});
        return entity;
    }
};
}  // namespace

// Two balls 200 units apart, heading at each other at 300 units/s each. Through each other, the left one ends a
// second later at x = 200; colliding, it stops near the impact point, x = -16.

TEST(PhysicsFiltering, BodiesThatWantEachOtherCollide) {
    PhysicsApp fixture;
    const Entity player = fixture.spawn({-100.0F, 0.0F}, kPlayer, kEnemy, kSpeed);
    const Entity enemy = fixture.spawn({100.0F, 0.0F}, kEnemy, kPlayer, -kSpeed);
    fixture.tick(kOneSecond);
    EXPECT_LT(fixture.xOf(player), 0.0F);
    EXPECT_GT(fixture.xOf(enemy), 0.0F);
}

TEST(PhysicsFiltering, BodiesThatIgnoreEachOtherPassThrough) {
    PhysicsApp fixture;
    const Entity player = fixture.spawn({-100.0F, 0.0F}, kPlayer, kEnemy | kWall, kSpeed);
    const Entity bullet = fixture.spawn({100.0F, 0.0F}, kPlayerBullet, kEnemy | kWall, -kSpeed);
    fixture.tick(kOneSecond);
    EXPECT_GT(fixture.xOf(player), 150.0F);
    EXPECT_LT(fixture.xOf(bullet), -150.0F);
}

TEST(PhysicsFiltering, AOneSidedMaskPassesThrough) {
    PhysicsApp fixture;
    const Entity player = fixture.spawn({-100.0F, 0.0F}, kPlayer, kEnemy, kSpeed);  // Wants enemies...
    const Entity enemy = fixture.spawn({100.0F, 0.0F}, kEnemy, kWall, -kSpeed);     // ...which ignore players.
    fixture.tick(kOneSecond);
    EXPECT_GT(fixture.xOf(player), 150.0F);
    EXPECT_LT(fixture.xOf(enemy), -150.0F);
}

TEST(PhysicsFiltering, AStaticWallOnlyStopsTheCategoriesItCollidesWith) {
    PhysicsApp fixture;
    fixture.spawnWall(kWall, kPlayer);
    const Entity player = fixture.spawn({-100.0F, 0.0F}, kPlayer, kWall, kSpeed);
    const Entity enemy = fixture.spawn({-100.0F, 100.0F}, kEnemy, kWall, kSpeed);
    fixture.tick(kOneSecond);
    EXPECT_LT(fixture.xOf(player), 0.0F);   // Stopped by the wall.
    EXPECT_GT(fixture.xOf(enemy), 150.0F);  // Through it.
}

TEST(PhysicsFiltering, ChangingTheMaskDuringPlayAppliesTheNewRule) {
    PhysicsApp fixture;
    const Entity player = fixture.spawn({-100.0F, 0.0F}, kPlayer, kWall, kSpeed);
    const Entity enemy = fixture.spawn({100.0F, 0.0F}, kEnemy, kPlayer, -kSpeed);
    fixture.tick(5);  // Still apart: 50 units travelled on each side.

    Collider collider = *fixture.components().get<Collider>(player);
    collider.mask = kEnemy;
    fixture.components().add(player, collider);  // The body is rebuilt with the new mask, keeping its speed.
    fixture.tick(kOneSecond);
    EXPECT_LT(fixture.xOf(player), 0.0F);
    EXPECT_GT(fixture.xOf(enemy), 0.0F);
}

TEST(PhysicsFiltering, TheDefaultColliderCollidesWithItself) {
    PhysicsApp fixture;
    const Collider defaults;
    const Entity left = fixture.spawn({-100.0F, 0.0F}, defaults.layer, defaults.mask, kSpeed);
    const Entity right = fixture.spawn({100.0F, 0.0F}, defaults.layer, defaults.mask, -kSpeed);
    fixture.tick(kOneSecond);
    EXPECT_LT(fixture.xOf(left), 0.0F);
    EXPECT_GT(fixture.xOf(right), 0.0F);
}

TEST(PhysicsFiltering, InvalidLayersAndMasksAreRejected) {
    PhysicsWorld world{PhysicsSettings{.maxBodies = 4, .maxBodyPairs = 4, .maxContactConstraints = 4}};
    const Entity entity{1, 1};
    const auto create = [&world, entity](std::uint32_t layer, std::uint32_t mask) {
        static_cast<void>(world.createBody(entity, Collider{.layer = layer, .mask = mask}, nullptr, Transform{}));
    };
    EXPECT_THROW(create(0U, kAllLayers), PhysicsException);                    // No category.
    EXPECT_THROW(create(1U << 15, kAllLayers), PhysicsException);              // A category beyond the 15.
    EXPECT_THROW(create(kPlayer, kAllLayers | (1U << 15)), PhysicsException);  // A mask bit beyond the 15.
    EXPECT_EQ(world.getBodyCount(), 0U);
    EXPECT_NO_THROW(create(kPlayer | kEnemy, 0U));  // A mask of 0 collides with nothing, but is valid.
}
