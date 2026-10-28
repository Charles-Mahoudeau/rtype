/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetIdTests
*/

#include <gtest/gtest.h>

#include <functional>

#include "core/asset/AssetId.hpp"

static_assert(sizeof(rtype::core::asset::AssetId) == 8);

TEST(AssetId, DefaultsToNoAsset) { EXPECT_EQ(rtype::core::asset::AssetId{}.value, 0U); }

TEST(AssetId, IsTheSameForTheSamePath) {
    EXPECT_EQ(rtype::core::asset::AssetId::fromPath("assets/ship.png"),
              rtype::core::asset::AssetId::fromPath("assets/ship.png"));
    EXPECT_NE(rtype::core::asset::AssetId::fromPath("assets/ship.png").value, 0U);
}

TEST(AssetId, DiffersBetweenPaths) {
    EXPECT_NE(rtype::core::asset::AssetId::fromPath("assets/ship.png"),
              rtype::core::asset::AssetId::fromPath("assets/enemy.png"));
}

TEST(AssetId, NormalizesThePath) {
    EXPECT_EQ(rtype::core::asset::AssetId::fromPath("assets/./sprites/../ship.png"),
              rtype::core::asset::AssetId::fromPath("assets/ship.png"));
}

TEST(AssetId, HashesAsItsValue) {
    const rtype::core::asset::AssetId id = rtype::core::asset::AssetId::fromPath("assets/ship.png");
    EXPECT_EQ(std::hash<rtype::core::asset::AssetId>{}(id), static_cast<std::size_t>(id.value));
}
