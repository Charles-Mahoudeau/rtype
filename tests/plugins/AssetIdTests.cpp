/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetIdTests
*/

#include <gtest/gtest.h>

#include <functional>

#include "plugins/backend/graphics/AssetId.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

static_assert(rtype::ecs::FieldTypeOf<rtype::plugins::backend::AssetId>::value == rtype::ecs::FieldType::kAssetId);
static_assert(sizeof(rtype::plugins::backend::AssetId) == 8);

TEST(AssetId, DefaultsToNoAsset) { EXPECT_EQ(rtype::plugins::backend::AssetId{}.value, 0U); }

TEST(AssetId, IsTheSameForTheSamePath) {
    EXPECT_EQ(rtype::plugins::backend::AssetId::fromPath("assets/ship.png"),
              rtype::plugins::backend::AssetId::fromPath("assets/ship.png"));
    EXPECT_NE(rtype::plugins::backend::AssetId::fromPath("assets/ship.png").value, 0U);
}

TEST(AssetId, DiffersBetweenPaths) {
    EXPECT_NE(rtype::plugins::backend::AssetId::fromPath("assets/ship.png"),
              rtype::plugins::backend::AssetId::fromPath("assets/enemy.png"));
}

TEST(AssetId, NormalizesThePath) {
    EXPECT_EQ(rtype::plugins::backend::AssetId::fromPath("assets/./sprites/../ship.png"),
              rtype::plugins::backend::AssetId::fromPath("assets/ship.png"));
}

TEST(AssetId, HashesAsItsValue) {
    const rtype::plugins::backend::AssetId id = rtype::plugins::backend::AssetId::fromPath("assets/ship.png");
    EXPECT_EQ(std::hash<rtype::plugins::backend::AssetId>{}(id), static_cast<std::size_t>(id.value));
}
