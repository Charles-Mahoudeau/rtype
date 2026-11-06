/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "rtype/contract/renderer/drawing/Sprite.hpp"
#include "rtype/contract/renderer/drawing/Text.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"

using rtype::contract::renderer::CompiledShader;
using rtype::contract::renderer::MaterialId;
using rtype::contract::renderer::ShaderTarget;
using rtype::contract::renderer::ShaderValue;
using rtype::contract::renderer::ShaderValueType;
using rtype::contract::renderer::ShaderVariant;
using rtype::contract::renderer::validate;

namespace {
/// @return A valid shader "flash" for sprites, with a vec4 tint in its material and a float flash per instance.
CompiledShader makeShader() {
    CompiledShader shader;
    shader.name = "flash";
    shader.variants.push_back(ShaderVariant{.target = ShaderTarget::kSprite, .vertex = {1, 2}, .fragment = {3}});
    shader.materialValues.push_back(ShaderValue{.name = "tint", .type = ShaderValueType::kVec4, .offset = 0});
    shader.materialSize = 16;
    shader.materialDefaults.resize(16);
    shader.instanceValues.push_back(ShaderValue{.name = "flash", .type = ShaderValueType::kFloat, .offset = 0});
    shader.textures.push_back({.name = "noise", .slot = 0});
    return shader;
}
}  // namespace

TEST(Shader, AValidShaderPasses) { EXPECT_NO_THROW(validate(makeShader())); }

TEST(Shader, FindsTheVariantOfATarget) {
    const CompiledShader shader = makeShader();
    ASSERT_NE(shader.findVariant(ShaderTarget::kSprite), nullptr);
    EXPECT_EQ(shader.findVariant(ShaderTarget::kSprite)->fragment, (std::vector<std::uint32_t>{3}));
    EXPECT_EQ(shader.findVariant(ShaderTarget::kGlyph), nullptr);
}

TEST(Shader, RejectsAShaderWithoutVariant) {
    CompiledShader shader = makeShader();
    shader.variants.clear();
    EXPECT_THROW(validate(shader), std::invalid_argument);
}

TEST(Shader, RejectsTwoVariantsForOneTarget) {
    CompiledShader shader = makeShader();
    shader.variants.push_back(shader.variants.front());
    EXPECT_THROW(validate(shader), std::invalid_argument);
}

TEST(Shader, RejectsAnEmptyStage) {
    CompiledShader shader = makeShader();
    shader.variants.front().fragment.clear();
    EXPECT_THROW(validate(shader), std::invalid_argument);
}

TEST(Shader, RejectsDefaultsOfTheWrongSize) {
    CompiledShader shader = makeShader();
    shader.materialDefaults.resize(8);
    EXPECT_THROW(validate(shader), std::invalid_argument);
}

TEST(Shader, RejectsValuesPastTheirBlock) {
    CompiledShader material = makeShader();
    material.materialValues.front().offset = 4;  // a vec4 from byte 4 ends at 20, past 16
    EXPECT_THROW(validate(material), std::invalid_argument);
    CompiledShader instance = makeShader();
    instance.instanceValues.front().offset = 64;
    EXPECT_THROW(validate(instance), std::invalid_argument);
}

TEST(Shader, RejectsTextureSlotsPastTheLimitOrShared) {
    CompiledShader past = makeShader();
    past.textures.front().slot = 3;
    EXPECT_THROW(validate(past), std::invalid_argument);
    CompiledShader shared = makeShader();
    shared.textures.push_back({.name = "palette", .slot = 0});
    EXPECT_THROW(validate(shared), std::invalid_argument);
}

TEST(Shader, DrawingsUseTheDefaultMaterialByDefault) {
    EXPECT_EQ(rtype::contract::renderer::Sprite{}.material, MaterialId{});
    EXPECT_EQ(rtype::contract::renderer::Text{}.material, MaterialId{});
    EXPECT_EQ(rtype::contract::renderer::ParticleDraw{}.material, MaterialId{});
    EXPECT_EQ(rtype::contract::renderer::Sprite{}.instanceValues.size() * sizeof(float) * 4,
              std::size_t{rtype::contract::renderer::kShaderInstanceBytes});
}
