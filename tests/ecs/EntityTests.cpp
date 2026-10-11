/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EntityTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <limits>
#include <type_traits>

#include "rtype/ecs/detail/Generation.hpp"
#include "rtype/ecs/entity/Entity.hpp"

static_assert(sizeof(rtype::ecs::Entity) == 8);
static_assert(std::is_trivially_copyable_v<rtype::ecs::Entity>);

TEST(Entity, DefaultIsNull) {
    const rtype::ecs::Entity entity{};
    EXPECT_TRUE(entity.isNull());
    EXPECT_EQ(entity.getIndex(), 0U);
    EXPECT_EQ(entity.getGeneration(), 0U);
    EXPECT_EQ(entity.getBits(), 0U);
}

TEST(Entity, KeepsIndexAndGeneration) {
    const rtype::ecs::Entity entity{42, 7};
    EXPECT_FALSE(entity.isNull());
    EXPECT_EQ(entity.getIndex(), 42U);
    EXPECT_EQ(entity.getGeneration(), 7U);
}

TEST(Entity, KeepsMaximumValues) {
    constexpr std::uint32_t kMax = std::numeric_limits<std::uint32_t>::max();
    const rtype::ecs::Entity entity{kMax, kMax};
    EXPECT_EQ(entity.getIndex(), kMax);
    EXPECT_EQ(entity.getGeneration(), kMax);
}

TEST(Entity, ComparesIndexAndGeneration) {
    EXPECT_EQ((rtype::ecs::Entity{3, 1}), (rtype::ecs::Entity{3, 1}));
    EXPECT_NE((rtype::ecs::Entity{3, 1}), (rtype::ecs::Entity{3, 2}));
    EXPECT_NE((rtype::ecs::Entity{3, 1}), (rtype::ecs::Entity{4, 1}));
}

TEST(Entity, HashDiffersAcrossGenerations) {
    const std::hash<rtype::ecs::Entity> hash{};
    EXPECT_EQ(hash(rtype::ecs::Entity{3, 1}), hash(rtype::ecs::Entity{3, 1}));
    EXPECT_NE(hash(rtype::ecs::Entity{3, 1}), hash(rtype::ecs::Entity{3, 2}));
}

TEST(Generation, IncrementsBelowMaximum) {
    EXPECT_EQ(rtype::ecs::detail::nextGeneration(1), 2U);
    EXPECT_EQ(rtype::ecs::detail::nextGeneration(std::numeric_limits<std::uint32_t>::max() - 1),
              std::numeric_limits<std::uint32_t>::max());
}

TEST(Generation, IsEmptyAtMaximum) {
    EXPECT_FALSE(rtype::ecs::detail::nextGeneration(std::numeric_limits<std::uint32_t>::max()));
}
