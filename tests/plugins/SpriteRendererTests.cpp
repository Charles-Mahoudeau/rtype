/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteRendererTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <glm/ext/vector_float2.hpp>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"
#include "plugins/sprite/SpriteRenderer.hpp"
#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"
#include "rtype/ecs/world/World.hpp"

static_assert(rtype::ecs::FieldTypeOf<rtype::core::asset::AssetId>::value == rtype::ecs::FieldType::kAssetId);
static_assert(rtype::ecs::Component<rtype::plugins::sprite::SpriteRenderer>);

TEST(SpriteRenderer, HasNeutralDefaults) {
    const rtype::plugins::sprite::SpriteRenderer sprite{};
    EXPECT_EQ(sprite.texture, rtype::core::asset::AssetId{});
    EXPECT_EQ(sprite.color, rtype::contract::renderer::Color{1.0F});
    EXPECT_EQ(sprite.origin, glm::vec2{0.0F});
    EXPECT_EQ(sprite.layer, 0);
    EXPECT_EQ(sprite.blend, rtype::contract::renderer::BlendMode::kAlpha);
    EXPECT_EQ(sprite.material, rtype::core::asset::AssetId{});
}

TEST(SpriteRenderer, ReflectsEveryFieldField) {
    using Traits = rtype::ecs::ComponentTraits<rtype::plugins::sprite::SpriteRenderer>;
    EXPECT_EQ(Traits::kName, "rtype.SpriteRenderer");
    ASSERT_EQ(Traits::kFields.size(), 7U);
    EXPECT_EQ(Traits::kFields[0].name, "texture");
    EXPECT_EQ(Traits::kFields[0].type, rtype::ecs::FieldType::kAssetId);
    EXPECT_EQ(Traits::kFields[1].name, "source");
    EXPECT_EQ(Traits::kFields[1].type, rtype::ecs::FieldType::kVec4);
    EXPECT_EQ(Traits::kFields[2].name, "color");
    EXPECT_EQ(Traits::kFields[2].type, rtype::ecs::FieldType::kVec4);
    EXPECT_EQ(Traits::kFields[3].name, "origin");
    EXPECT_EQ(Traits::kFields[3].type, rtype::ecs::FieldType::kVec2);
    EXPECT_EQ(Traits::kFields[4].name, "layer");
    EXPECT_EQ(Traits::kFields[4].type, rtype::ecs::FieldType::kI32);
    EXPECT_EQ(Traits::kFields[5].name, "blend");
    EXPECT_EQ(Traits::kFields[5].type, rtype::ecs::FieldType::kU8);
    EXPECT_EQ(Traits::kFields[6].name, "material");
    EXPECT_EQ(Traits::kFields[6].type, rtype::ecs::FieldType::kAssetId);
}

TEST(SpriteRenderer, ReflectsTheSourceAsItsPositionThenItsSize) {
    rtype::plugins::sprite::SpriteRenderer sprite{
        .source = rtype::contract::renderer::math::Rect{{1.0F, 2.0F}, {3.0F, 4.0F}}};
    const auto& source = rtype::ecs::ComponentTraits<rtype::plugins::sprite::SpriteRenderer>::kFields[1];
    std::array<float, 4> read{};
    std::memcpy(read.data(), reinterpret_cast<const std::byte*>(&sprite) + source.offset, sizeof(read));  // NOLINT
    EXPECT_EQ(read, (std::array<float, 4>{1.0F, 2.0F, 3.0F, 4.0F}));
}

TEST(SpriteRenderer, IsStoredOnAnEntity) {
    rtype::ecs::World world;
    const rtype::ecs::Entity ship = world.spawn();
    world.getComponents().add(ship, rtype::plugins::sprite::SpriteRenderer{
                                        .texture = rtype::core::asset::AssetId::fromPath("ship.png"),
                                        .source = rtype::contract::renderer::math::Rect{{0.0F, 0.0F}, {32.0F, 16.0F}},
                                        .layer = 10});
    const auto* sprite = world.getComponents().get<rtype::plugins::sprite::SpriteRenderer>(ship);
    ASSERT_NE(sprite, nullptr);
    EXPECT_EQ(sprite->texture, rtype::core::asset::AssetId::fromPath("ship.png"));
    EXPECT_FLOAT_EQ(sprite->source.getWidth(), 32.0F);
    EXPECT_EQ(sprite->layer, 10);
}
