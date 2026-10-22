/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsLayersTests
*/

#include <Jolt/Jolt.h>  // IWYU pragma: keep
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <gtest/gtest.h>

#include <cstdint>

#include "plugins/physics/detail/Layers.hpp"

namespace layers = rtype::plugins::physics::detail::layers;
namespace broadPhaseLayers = rtype::plugins::physics::detail::broadPhaseLayers;
using rtype::plugins::physics::detail::BroadPhaseLayers;
using rtype::plugins::physics::detail::ObjectLayerPairFilter;
using rtype::plugins::physics::detail::ObjectVsBroadPhaseFilter;

namespace {
constexpr std::uint32_t kPlayer = 1U << 0;
constexpr std::uint32_t kPlayerBullet = 1U << 1;
constexpr std::uint32_t kEnemy = 1U << 2;
constexpr std::uint32_t kWall = 1U << 3;
}  // namespace

TEST(PhysicsLayers, EncodingKeepsEveryCategoryAndMaskBit) {
    for (std::uint32_t bit = 0; bit < layers::kCategoryCount; ++bit) {
        const std::uint32_t category = 1U << bit;
        const std::uint32_t mask = layers::kAllCategories & ~category;
        for (const bool isStatic : {false, true}) {
            const JPH::ObjectLayer layer = layers::encode(category, mask, isStatic);
            EXPECT_EQ(layers::categoriesOf(layer), category) << bit;
            EXPECT_EQ(layers::maskOf(layer), mask) << bit;
            EXPECT_EQ(layers::isStatic(layer), isStatic) << bit;
        }
    }
}

TEST(PhysicsLayers, ALayerIsNeverJoltsInvalidLayer) {
    EXPECT_NE(layers::encode(layers::kAllCategories, layers::kAllCategories, true), JPH::cObjectLayerInvalid);
    EXPECT_NE(layers::encode(layers::kAllCategories, layers::kAllCategories, false), JPH::cObjectLayerInvalid);
}

TEST(PhysicsLayers, EncodingDropsBitsBeyondTheCategories) {
    const JPH::ObjectLayer layer = layers::encode(0xFFFFFFFFU, 0xFFFFFFFFU, false);
    EXPECT_EQ(layers::categoriesOf(layer), layers::kAllCategories);
    EXPECT_EQ(layers::maskOf(layer), layers::kAllCategories);
    EXPECT_FALSE(layers::isStatic(layer));
}

TEST(PhysicsLayers, ValidityNeedsACategoryAndNoExtraBit) {
    EXPECT_TRUE(layers::isValid(kPlayer, layers::kAllCategories));
    EXPECT_TRUE(layers::isValid(kPlayer | kEnemy, 0U));  // A mask of 0 collides with nothing, but is allowed.
    EXPECT_FALSE(layers::isValid(0U, layers::kAllCategories));
    EXPECT_FALSE(layers::isValid(1U << layers::kCategoryCount, layers::kAllCategories));
    EXPECT_FALSE(layers::isValid(kPlayer, 1U << layers::kCategoryCount));
}

TEST(PhysicsLayers, BodiesCollideWhenEachOneWantsTheOther) {
    const JPH::ObjectLayer player = layers::encode(kPlayer, kEnemy | kWall, false);
    const JPH::ObjectLayer enemy = layers::encode(kEnemy, kPlayer | kPlayerBullet | kWall, false);
    const JPH::ObjectLayer bullet = layers::encode(kPlayerBullet, kEnemy | kWall, false);
    EXPECT_TRUE(layers::canCollide(player, enemy));
    EXPECT_TRUE(layers::canCollide(bullet, enemy));
    EXPECT_FALSE(layers::canCollide(player, bullet));  // Neither wants the other.
}

TEST(PhysicsLayers, AOneSidedMaskIsNotEnough) {
    const JPH::ObjectLayer wantsEnemies = layers::encode(kPlayer, kEnemy, false);
    const JPH::ObjectLayer ignoresPlayers = layers::encode(kEnemy, kWall, false);
    EXPECT_FALSE(layers::canCollide(wantsEnemies, ignoresPlayers));
    EXPECT_FALSE(layers::canCollide(ignoresPlayers, wantsEnemies));
}

TEST(PhysicsLayers, TwoStaticBodiesNeverCollide) {
    const JPH::ObjectLayer wall = layers::encode(kWall, layers::kAllCategories, true);
    EXPECT_FALSE(layers::canCollide(wall, wall));
}

TEST(PhysicsLayers, AStaticBodyFollowsTheMasks) {
    const JPH::ObjectLayer wall = layers::encode(kWall, kPlayer, true);
    EXPECT_TRUE(layers::canCollide(wall, layers::encode(kPlayer, kWall, false)));
    EXPECT_FALSE(layers::canCollide(wall, layers::encode(kEnemy, kWall, false)));
}

TEST(PhysicsLayers, EveryCategoryAndMaskCollidesWithEverythingButStatics) {
    const JPH::ObjectLayer moving = layers::encode(layers::kAllCategories, layers::kAllCategories, false);
    const JPH::ObjectLayer still = layers::encode(layers::kAllCategories, layers::kAllCategories, true);
    EXPECT_TRUE(layers::canCollide(moving, moving));
    EXPECT_TRUE(layers::canCollide(moving, still));
    EXPECT_FALSE(layers::canCollide(still, still));
}

TEST(PhysicsLayers, TheStaticFlagPicksTheBroadPhaseTree) {
    const BroadPhaseLayers broadPhase;
    EXPECT_EQ(broadPhase.GetNumBroadPhaseLayers(), broadPhaseLayers::kCount);
    EXPECT_EQ(broadPhase.GetBroadPhaseLayer(layers::encode(kWall, kPlayer, true)), broadPhaseLayers::kNonMoving);
    EXPECT_EQ(broadPhase.GetBroadPhaseLayer(layers::encode(kPlayer, kWall, false)), broadPhaseLayers::kMoving);
}

TEST(PhysicsLayers, StaticBodiesSkipTheStaticTree) {
    const ObjectVsBroadPhaseFilter filter;
    const JPH::ObjectLayer wall = layers::encode(kWall, layers::kAllCategories, true);
    const JPH::ObjectLayer player = layers::encode(kPlayer, layers::kAllCategories, false);
    EXPECT_FALSE(filter.ShouldCollide(wall, broadPhaseLayers::kNonMoving));
    EXPECT_TRUE(filter.ShouldCollide(wall, broadPhaseLayers::kMoving));
    EXPECT_TRUE(filter.ShouldCollide(player, broadPhaseLayers::kNonMoving));
    EXPECT_TRUE(filter.ShouldCollide(player, broadPhaseLayers::kMoving));
}

TEST(PhysicsLayers, ThePairFilterAppliesTheRule) {
    const ObjectLayerPairFilter filter;
    const JPH::ObjectLayer player = layers::encode(kPlayer, kEnemy, false);
    const JPH::ObjectLayer enemy = layers::encode(kEnemy, kPlayer, false);
    const JPH::ObjectLayer bullet = layers::encode(kPlayerBullet, kEnemy, false);
    EXPECT_TRUE(filter.ShouldCollide(player, enemy));
    EXPECT_FALSE(filter.ShouldCollide(player, bullet));
}
