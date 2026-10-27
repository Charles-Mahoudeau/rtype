/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteRendererTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/world/World.hpp"

static_assert(rtype::ecs::Component<rtype::plugins::backend::SpriteRenderer>);

TEST(SpriteRenderer, HasNeutralDefaults) {
    const rtype::plugins::backend::SpriteRenderer sprite{};
    EXPECT_EQ(sprite.texture, rtype::plugins::asset::AssetId{});
    EXPECT_EQ(sprite.color, rtype::plugins::backend::graphics::Color{1.0F});
    EXPECT_EQ(sprite.origin, glm::vec2{0.0F});
    EXPECT_EQ(sprite.layer, 0);
}

TEST(SpriteRenderer, ReflectsEveryFieldButTheSource) {
    using Traits = rtype::ecs::ComponentTraits<rtype::plugins::backend::SpriteRenderer>;
    EXPECT_EQ(Traits::kName, "rtype.SpriteRenderer");
    ASSERT_EQ(Traits::kFields.size(), 4U);
    EXPECT_EQ(Traits::kFields[0].name, "texture");
    EXPECT_EQ(Traits::kFields[0].type, rtype::ecs::FieldType::kAssetId);
    EXPECT_EQ(Traits::kFields[1].name, "color");
    EXPECT_EQ(Traits::kFields[1].type, rtype::ecs::FieldType::kVec4);
    EXPECT_EQ(Traits::kFields[2].name, "origin");
    EXPECT_EQ(Traits::kFields[2].type, rtype::ecs::FieldType::kVec2);
    EXPECT_EQ(Traits::kFields[3].name, "layer");
    EXPECT_EQ(Traits::kFields[3].type, rtype::ecs::FieldType::kI32);
}

TEST(SpriteRenderer, IsStoredOnAnEntity) {
    rtype::ecs::World world;
    const rtype::ecs::Entity ship = world.spawn();
    world.getComponents().add(ship, rtype::plugins::backend::SpriteRenderer{
                                        .texture = rtype::plugins::asset::AssetId::fromPath("ship.png"),
                                        .source = rtype::plugins::backend::math::Rect{{0.0F, 0.0F}, {32.0F, 16.0F}},
                                        .layer = 10});
    const auto* sprite = world.getComponents().get<rtype::plugins::backend::SpriteRenderer>(ship);
    ASSERT_NE(sprite, nullptr);
    EXPECT_EQ(sprite->texture, rtype::plugins::asset::AssetId::fromPath("ship.png"));
    EXPECT_FLOAT_EQ(sprite->source.getWidth(), 32.0F);
    EXPECT_EQ(sprite->layer, 10);
}
