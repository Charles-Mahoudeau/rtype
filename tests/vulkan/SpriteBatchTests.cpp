/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteBatchTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "render/vulkan/rendering/Blend.hpp"
#include "render/vulkan/rendering/SpriteBatch.hpp"

namespace {
using rtype::render::vulkan::rendering::SpriteBatch;
using rtype::render::vulkan::rendering::SpriteInstance;
using TextureId = rtype::plugins::backend::graphics::TextureId;

constexpr TextureId kShips{.index = 0, .generation = 1};
constexpr TextureId kEnemies{.index = 1, .generation = 1};
}  // namespace

TEST(SpriteInstance, TakesItsUvRectFromTheSourceAndTheTextureSize) {
    const rtype::plugins::backend::graphics::Sprite sprite{
        .texture = kShips, .source = rtype::plugins::backend::math::Rect{{32.0F, 0.0F}, {32.0F, 16.0F}}};
    const SpriteInstance instance = SpriteInstance::fromSprite(sprite, {128, 64});
    EXPECT_EQ(instance.uvRect, (glm::vec4{0.25F, 0.0F, 0.5F, 0.25F}));
    EXPECT_EQ(instance.size, (glm::vec2{32.0F, 16.0F}));
}

TEST(SpriteInstance, ScalesItsSizeAndOrigin) {
    const rtype::plugins::backend::graphics::Sprite sprite{
        .texture = kShips,
        .source = rtype::plugins::backend::math::Rect{{0.0F, 0.0F}, {10.0F, 20.0F}},
        .position = {5.0F, 6.0F},
        .scale = {2.0F, -1.0F},
        .origin = {5.0F, 10.0F},
        .rotation = 0.5F,
        .color = {1.0F, 0.0F, 0.0F, 0.5F}};
    const SpriteInstance instance = SpriteInstance::fromSprite(sprite, {10, 20});
    EXPECT_EQ(instance.position, (glm::vec2{5.0F, 6.0F}));
    EXPECT_EQ(instance.size, (glm::vec2{20.0F, -20.0F}));
    EXPECT_EQ(instance.origin, (glm::vec2{10.0F, -10.0F}));
    EXPECT_FLOAT_EQ(instance.rotation, 0.5F);
    EXPECT_EQ(instance.color, (glm::vec4{1.0F, 0.0F, 0.0F, 0.5F}));
}

TEST(SpriteInstance, DescribesOnePerInstanceVertex) {
    const auto bindings = SpriteInstance::getBindings(2);
    EXPECT_EQ(bindings[0].binding, 2U);
    EXPECT_EQ(bindings[0].stride, sizeof(SpriteInstance));
    EXPECT_EQ(bindings[0].inputRate, vk::VertexInputRate::eInstance);
    const auto attributes = SpriteInstance::getAttributes(2);
    for (std::uint32_t location = 0; location < attributes.size(); ++location) {
        EXPECT_EQ(attributes.at(location).location, location);
        EXPECT_EQ(attributes.at(location).binding, 2U);
    }
    EXPECT_EQ(attributes[3].format, vk::Format::eR32Sfloat);
    EXPECT_EQ(attributes[4].offset, 32U);
    EXPECT_EQ(attributes[5].offset, 48U);
    EXPECT_EQ(attributes[6].format, vk::Format::eR32Uint);  // The blend mode is an integer.
    EXPECT_EQ(attributes[6].offset, 28U);
}

TEST(SpriteInstance, CarriesItsBlendModeAsTheShadersReadIt) {
    using rtype::plugins::backend::graphics::BlendMode;
    using rtype::plugins::backend::graphics::Sprite;
    EXPECT_EQ(SpriteInstance::fromSprite(Sprite{}, {1, 1}).blend, rtype::render::vulkan::rendering::kBlendAlpha);
    EXPECT_EQ(SpriteInstance::fromSprite(Sprite{.blend = BlendMode::kAdditive}, {1, 1}).blend,
              rtype::render::vulkan::rendering::kBlendAdditive);
    EXPECT_EQ(SpriteInstance::fromSprite(Sprite{.blend = BlendMode::kCutout}, {1, 1}).blend,
              rtype::render::vulkan::rendering::kBlendCutout);
}

TEST(SpriteBatch, MergesConsecutiveSpritesOfATexture) {
    SpriteBatch batch;
    batch.add(kShips, {});
    batch.add(kShips, {});
    batch.add(kEnemies, {});
    ASSERT_EQ(batch.getBatches().size(), 2U);
    EXPECT_EQ(batch.getBatches()[0].texture, kShips);
    EXPECT_EQ(batch.getBatches()[0].firstInstance, 0U);
    EXPECT_EQ(batch.getBatches()[0].instanceCount, 2U);
    EXPECT_EQ(batch.getBatches()[1].texture, kEnemies);
    EXPECT_EQ(batch.getBatches()[1].firstInstance, 2U);
    EXPECT_EQ(batch.getBatches()[1].instanceCount, 1U);
    EXPECT_EQ(batch.getInstances().size(), 3U);
}

TEST(SpriteBatch, KeepsTheDrawOrderAcrossTextures) {
    SpriteBatch batch;
    batch.add(kShips, {});
    batch.add(kEnemies, {});
    batch.add(kShips, {});
    ASSERT_EQ(batch.getBatches().size(), 3U);
    EXPECT_EQ(batch.getBatches()[2].texture, kShips);
    EXPECT_EQ(batch.getBatches()[2].firstInstance, 2U);
}

TEST(SpriteBatch, ClearForgetsEverything) {
    SpriteBatch batch;
    batch.add(kShips, {});
    batch.clear();
    EXPECT_TRUE(batch.getInstances().empty());
    EXPECT_TRUE(batch.getBatches().empty());
    batch.add(kShips, {});
    EXPECT_EQ(batch.getBatches().size(), 1U);
}
