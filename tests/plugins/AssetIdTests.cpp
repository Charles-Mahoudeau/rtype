/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetIdTests
*/

#include <gtest/gtest.h>

#include <functional>

#include "plugins/asset/AssetId.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

static_assert(rtype::ecs::FieldTypeOf<rtype::plugins::asset::AssetId>::value == rtype::ecs::FieldType::kAssetId);
static_assert(sizeof(rtype::plugins::asset::AssetId) == 8);

TEST(AssetId, DefaultsToNoAsset) { EXPECT_EQ(rtype::plugins::asset::AssetId{}.value, 0U); }

TEST(AssetId, IsTheSameForTheSamePath) {
    EXPECT_EQ(rtype::plugins::asset::AssetId::fromPath("assets/ship.png"),
              rtype::plugins::asset::AssetId::fromPath("assets/ship.png"));
    EXPECT_NE(rtype::plugins::asset::AssetId::fromPath("assets/ship.png").value, 0U);
}

TEST(AssetId, DiffersBetweenPaths) {
    EXPECT_NE(rtype::plugins::asset::AssetId::fromPath("assets/ship.png"),
              rtype::plugins::asset::AssetId::fromPath("assets/enemy.png"));
}

TEST(AssetId, NormalizesThePath) {
    EXPECT_EQ(rtype::plugins::asset::AssetId::fromPath("assets/./sprites/../ship.png"),
              rtype::plugins::asset::AssetId::fromPath("assets/ship.png"));
}

TEST(AssetId, HashesAsItsValue) {
    const rtype::plugins::asset::AssetId id = rtype::plugins::asset::AssetId::fromPath("assets/ship.png");
    EXPECT_EQ(std::hash<rtype::plugins::asset::AssetId>{}(id), static_cast<std::size_t>(id.value));
}
