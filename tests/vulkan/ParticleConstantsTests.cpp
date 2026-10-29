/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleConstantsTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2.hpp>

#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "render/vulkan/rendering/Blend.hpp"
#include "render/vulkan/rendering/ParticleConstants.hpp"

using rtype::plugins::backend::graphics::BlendMode;
using rtype::plugins::backend::graphics::ParticleAlignment;
using rtype::plugins::backend::graphics::ParticleDraw;
using rtype::plugins::backend::math::Rect;
using rtype::render::vulkan::rendering::ParticleConstants;

TEST(ParticleConstants, AnEmptySourceShowsTheWholeTexture) {
    const ParticleConstants constants = ParticleConstants::fromDraw(ParticleDraw{}, glm::uvec2{64, 32});
    EXPECT_EQ(constants.uvRect, (glm::vec4{0.0F, 0.0F, 1.0F, 1.0F}));
}

TEST(ParticleConstants, TakesItsUvRectFromTheSourceAndTheTextureSize) {
    ParticleDraw draw;
    draw.source = Rect{{16.0F, 8.0F}, {16.0F, 16.0F}};
    const ParticleConstants constants = ParticleConstants::fromDraw(draw, glm::uvec2{64, 32});
    EXPECT_FLOAT_EQ(constants.uvRect.x, 0.25F);
    EXPECT_FLOAT_EQ(constants.uvRect.y, 0.25F);
    EXPECT_FLOAT_EQ(constants.uvRect.z, 0.5F);
    EXPECT_FLOAT_EQ(constants.uvRect.w, 0.75F);
}

TEST(ParticleConstants, AnEmptyTextureShowsItWhole) {
    ParticleDraw draw;
    draw.source = Rect{{16.0F, 8.0F}, {16.0F, 16.0F}};
    EXPECT_EQ(ParticleConstants::fromDraw(draw, glm::uvec2{0, 0}).uvRect, (glm::vec4{0.0F, 0.0F, 1.0F, 1.0F}));
}

TEST(ParticleConstants, CarriesTheBlendModeAndTheAlignment) {
    ParticleDraw draw;
    draw.blend = BlendMode::kCutout;
    draw.alignment = ParticleAlignment::kVelocity;
    const ParticleConstants constants = ParticleConstants::fromDraw(draw, glm::uvec2{8, 8});
    EXPECT_EQ(constants.blend, rtype::render::vulkan::rendering::kBlendCutout);
    EXPECT_EQ(constants.alignment, 1U);
}
