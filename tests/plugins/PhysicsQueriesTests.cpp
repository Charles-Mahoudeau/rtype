/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsQueriesTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <vector>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/entity/Entity.hpp"

using rtype::ecs::Entity;
using rtype::plugins::physics::Collider;
using rtype::plugins::physics::PhysicsSettings;
using rtype::plugins::physics::PhysicsWorld;
using rtype::plugins::physics::QueryFilter;
using rtype::plugins::physics::ShapeType;
using rtype::plugins::physics::exceptions::PhysicsException;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kTolerance = 0.05F;
constexpr std::uint32_t kWallLayer = 1U << 1;
constexpr std::uint32_t kOtherLayer = 1U << 2;

/// @brief A world with two static walls across the +X axis: their left faces at x = 90 and x = 190.
struct Walls {
    PhysicsWorld world{PhysicsSettings{.unitsPerMeter = 100.0F,
                                       .maxBodies = 16,
                                       .maxBodyPairs = 16,
                                       .maxContactConstraints = 16,
                                       .tempAllocatorBytes = 1024 * 1024}};
    Entity near{1, 1};  ///< The wall at x = 100 (faces at 90 and 110).
    Entity far{2, 1};   ///< The wall at x = 200 (faces at 190 and 210), in another layer.

    Walls() {
        static_cast<void>(
            world.createBody(near, Collider{.shape = ShapeType::kBox, .size = {20.0F, 200.0F}, .layer = kWallLayer},
                             nullptr, Transform{.position = {100.0F, 0.0F}}));
        static_cast<void>(
            world.createBody(far, Collider{.shape = ShapeType::kBox, .size = {20.0F, 200.0F}, .layer = kOtherLayer},
                             nullptr, Transform{.position = {200.0F, 0.0F}}));
    }

    /// @brief Adds a static circle of the radius at the position.
    void addCircle(Entity entity, glm::vec2 position, float radius, bool sensor = false) {
        static_cast<void>(world.createBody(entity,
                                           Collider{.shape = ShapeType::kCircle, .radius = radius, .sensor = sensor},
                                           nullptr, Transform{.position = position}));
    }
};
}  // namespace

TEST(PhysicsQueries, ARayHitsTheNearestBody) {
    const Walls walls;  // Never stepped: queries see the bodies as soon as they are created.
    const std::optional<PhysicsWorld::RayHit> hit = walls.world.raycast({0.0F, 0.0F}, {1.0F, 0.0F}, 1000.0F);
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, walls.near);
    EXPECT_NEAR(hit->point.x, 90.0F, kTolerance);
    EXPECT_NEAR(hit->point.y, 0.0F, kTolerance);
    EXPECT_NEAR(hit->normal.x, -1.0F, kTolerance);  // Out of the wall, back toward the origin.
    EXPECT_NEAR(hit->normal.y, 0.0F, kTolerance);
    EXPECT_NEAR(hit->distance, 90.0F, kTolerance);
}

TEST(PhysicsQueries, ARayStopsAtItsMaxDistance) {
    const Walls walls;
    EXPECT_FALSE(walls.world.raycast({0.0F, 0.0F}, {1.0F, 0.0F}, 50.0F));
}

TEST(PhysicsQueries, TheLengthOfTheDirectionDoesNotMatter) {
    const Walls walls;
    const std::optional<PhysicsWorld::RayHit> hit = walls.world.raycast({0.0F, 0.0F}, {25.0F, 0.0F}, 1000.0F);
    ASSERT_TRUE(hit);
    EXPECT_NEAR(hit->distance, 90.0F, kTolerance);
}

TEST(PhysicsQueries, RaysFollowTheYDownAxes) {
    Walls walls;
    const Entity floor{3, 1};
    static_cast<void>(walls.world.createBody(floor, Collider{.shape = ShapeType::kBox, .size = {40.0F, 20.0F}}, nullptr,
                                             Transform{.position = {0.0F, 100.0F}}));
    const std::optional<PhysicsWorld::RayHit> hit = walls.world.raycast({0.0F, 0.0F}, {0.0F, 1.0F}, 1000.0F);
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, floor);
    EXPECT_NEAR(hit->point.y, 90.0F, kTolerance);   // Down the screen.
    EXPECT_NEAR(hit->normal.y, -1.0F, kTolerance);  // Back up, toward the origin.
}

TEST(PhysicsQueries, RaycastAllReturnsEveryBodyClosestFirst) {
    const Walls walls;
    const std::vector<PhysicsWorld::RayHit> hits = walls.world.raycastAll({0.0F, 0.0F}, {1.0F, 0.0F}, 1000.0F);
    ASSERT_EQ(hits.size(), 2U);
    EXPECT_EQ(hits.at(0).entity, walls.near);
    EXPECT_NEAR(hits.at(0).distance, 90.0F, kTolerance);
    EXPECT_EQ(hits.at(1).entity, walls.far);
    EXPECT_NEAR(hits.at(1).distance, 190.0F, kTolerance);
}

TEST(PhysicsQueries, TheMaskPicksTheCategoriesSeen) {
    const Walls walls;
    const std::optional<PhysicsWorld::RayHit> hit =
        walls.world.raycast({0.0F, 0.0F}, {1.0F, 0.0F}, 1000.0F, QueryFilter{.mask = kOtherLayer});
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, walls.far);  // Through the near wall, which is not in the mask.
}

TEST(PhysicsQueries, TheIgnoredEntityIsSkipped) {
    Walls walls;
    const Entity shooter{4, 1};
    walls.addCircle(shooter, {0.0F, 0.0F}, 16.0F);
    const std::optional<PhysicsWorld::RayHit> fromInside = walls.world.raycast({0.0F, 0.0F}, {1.0F, 0.0F}, 1000.0F);
    ASSERT_TRUE(fromInside);
    EXPECT_EQ(fromInside->entity, shooter);  // A ray starting inside a body hits it at once...
    EXPECT_NEAR(fromInside->distance, 0.0F, kTolerance);

    const std::optional<PhysicsWorld::RayHit> hit =
        walls.world.raycast({0.0F, 0.0F}, {1.0F, 0.0F}, 1000.0F, QueryFilter{.ignore = shooter});
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, walls.near);  // ...unless it is the ignored shooter.
}

TEST(PhysicsQueries, SensorsAreOnlySeenWhenAskedFor) {
    Walls walls;
    const Entity zone{5, 1};
    walls.addCircle(zone, {50.0F, 0.0F}, 10.0F, true);
    const std::optional<PhysicsWorld::RayHit> hit = walls.world.raycast({0.0F, 0.0F}, {1.0F, 0.0F}, 1000.0F);
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, walls.near);

    const std::optional<PhysicsWorld::RayHit> withSensors =
        walls.world.raycast({0.0F, 0.0F}, {1.0F, 0.0F}, 1000.0F, QueryFilter{.includeSensors = true});
    ASSERT_TRUE(withSensors);
    EXPECT_EQ(withSensors->entity, zone);
    EXPECT_NEAR(withSensors->distance, 40.0F, kTolerance);
}

TEST(PhysicsQueries, AShapeCastStopsWhereTheShapeFirstTouches) {
    const Walls walls;
    const std::optional<PhysicsWorld::RayHit> hit = walls.world.shapeCast(
        Collider{.shape = ShapeType::kCircle, .radius = 10.0F}, {0.0F, 0.0F}, {1.0F, 0.0F}, 1000.0F);
    ASSERT_TRUE(hit);
    EXPECT_EQ(hit->entity, walls.near);
    EXPECT_NEAR(hit->distance, 80.0F, 0.5F);  // The circle's edge reaches the face at 90 when its center is at 80.
    EXPECT_NEAR(hit->point.x, 90.0F, 0.5F);
    EXPECT_NEAR(hit->normal.x, -1.0F, kTolerance);
    EXPECT_NEAR(hit->normal.y, 0.0F, kTolerance);
}

TEST(PhysicsQueries, OverlapListsTheEntitiesUnderAShape) {
    const Walls walls;
    const Collider probe{.shape = ShapeType::kCircle, .radius = 20.0F};
    EXPECT_EQ(walls.world.overlap(probe, {100.0F, 0.0F}), std::vector<Entity>{walls.near});
    EXPECT_TRUE(walls.world.overlap(probe, {0.0F, 0.0F}).empty());
    EXPECT_TRUE(walls.world.overlap(probe, {100.0F, 0.0F}, QueryFilter{.mask = kOtherLayer}).empty());

    const Collider wide{.shape = ShapeType::kBox, .size = {200.0F, 20.0F}};
    const std::vector<Entity> both = walls.world.overlap(wide, {150.0F, 0.0F});
    EXPECT_EQ(both.size(), 2U);
}

TEST(PhysicsQueries, OverlapPlacesAnOffsetShapeWhereItIs) {
    const Walls walls;
    const Collider offset{.shape = ShapeType::kCircle, .radius = 5.0F, .offset = {100.0F, 0.0F}};
    EXPECT_EQ(walls.world.overlap(offset, {0.0F, 0.0F}), std::vector<Entity>{walls.near});
}

TEST(PhysicsQueries, DegenerateQueriesFindNothing) {
    const Walls walls;
    EXPECT_FALSE(walls.world.raycast({0.0F, 0.0F}, {0.0F, 0.0F}, 1000.0F));
    EXPECT_FALSE(walls.world.raycast({0.0F, 0.0F}, {1.0F, 0.0F}, 0.0F));
    EXPECT_TRUE(walls.world.raycastAll({0.0F, 0.0F}, {0.0F, 0.0F}, 1000.0F).empty());
    EXPECT_FALSE(walls.world.shapeCast(Collider{}, {0.0F, 0.0F}, {1.0F, 0.0F}, -1.0F));
}

TEST(PhysicsQueries, AnInvalidShapeThrows) {
    const Walls walls;
    const Collider flat{.shape = ShapeType::kCircle, .radius = 0.0F};
    EXPECT_THROW(static_cast<void>(walls.world.shapeCast(flat, {0.0F, 0.0F}, {1.0F, 0.0F}, 100.0F)), PhysicsException);
    EXPECT_THROW(static_cast<void>(walls.world.overlap(flat, {0.0F, 0.0F})), PhysicsException);
}
