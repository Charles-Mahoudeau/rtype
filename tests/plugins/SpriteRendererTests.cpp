/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteRendererTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "plugins/backend/SpriteRenderer.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/world/World.hpp"

static_assert(rtype::ecs::Component<rtype::plugins::backend::SpriteRenderer>);

TEST(SpriteRenderer, HasNeutralDefaults) {
    const rtype::plugins::backend::SpriteRenderer sprite{};
    EXPECT_EQ(sprite.texture, rtype::plugins::backend::graphics::TextureId{});
    EXPECT_EQ(sprite.color, rtype::plugins::backend::graphics::Color{1.0F});
    EXPECT_EQ(sprite.origin, glm::vec2{0.0F});
    EXPECT_EQ(sprite.layer, 0);
}

TEST(SpriteRenderer, IsAComponentWithoutReflectedFields) {
    EXPECT_EQ(rtype::ecs::ComponentTraits<rtype::plugins::backend::SpriteRenderer>::kName, "rtype.SpriteRenderer");
    EXPECT_TRUE(rtype::ecs::ComponentTraits<rtype::plugins::backend::SpriteRenderer>::kFields.empty());
}

TEST(SpriteRenderer, IsStoredOnAnEntity) {
    rtype::ecs::World world;
    const rtype::ecs::Entity ship = world.spawn();
    world.getComponents().add(ship, rtype::plugins::backend::SpriteRenderer{
                                        .texture = {.index = 3, .generation = 1},
                                        .source = rtype::plugins::backend::math::Rect{{0.0F, 0.0F}, {32.0F, 16.0F}},
                                        .layer = 10});
    const auto* sprite = world.getComponents().get<rtype::plugins::backend::SpriteRenderer>(ship);
    ASSERT_NE(sprite, nullptr);
    EXPECT_EQ(sprite->texture, (rtype::plugins::backend::graphics::TextureId{.index = 3, .generation = 1}));
    EXPECT_FLOAT_EQ(sprite->source.getWidth(), 32.0F);
    EXPECT_EQ(sprite->layer, 10);
}
