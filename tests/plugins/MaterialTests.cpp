/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/detail/ShaderCompiler.hpp"
#include "plugins/shader/exceptions/ShaderExceptions.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::contract::renderer::CompiledShader;
using rtype::core::asset::AssetId;
using rtype::plugins::shader::Material;
using rtype::plugins::shader::MaterialAssets;
using rtype::plugins::shader::MaterialInstance;
using rtype::plugins::shader::packMaterialValues;
using rtype::plugins::shader::resolveMaterialTextures;
using rtype::plugins::shader::exceptions::ShaderException;

static_assert(rtype::ecs::Resource<MaterialAssets>);
static_assert(rtype::ecs::Component<MaterialInstance>);

namespace {
/// @return A shader with a float speed (offset 0, default 2), a vec4 tint (offset 16), an int frames (offset 32), an
/// instance value and two textures.
CompiledShader makeShader() {
    const std::array targets{rtype::contract::renderer::ShaderTarget::kSprite};
    return rtype::plugins::shader::detail::compileShader(
        "uniform float speed = 2.0;\nuniform vec4 tint;\nuniform int frames = 4;\ninstance uniform float flash;\n"
        "uniform sampler2D noise;\nuniform sampler2D palette;\n"
        "void fragment() { COLOR *= tint * speed * float(frames) * flash * texture(noise, UV) * texture(palette, UV); "
        "}",
        "fx", targets);
}

/// @return The float at @p offset of @p block.
float floatAt(const std::vector<std::byte>& block, std::size_t offset) {
    float value = 0.0F;
    std::memcpy(&value, &block.at(offset), sizeof(value));
    return value;
}
}  // namespace

TEST(Material, SetReplacesAValueOfTheSameName) {
    Material material;
    material.set("speed", 1.0F).set("speed", 3.0F).setTexture("noise", AssetId{1}).setTexture("noise", AssetId{2});
    ASSERT_EQ(material.values.size(), 1U);
    EXPECT_EQ(std::get<float>(material.values.front().value), 3.0F);
    ASSERT_EQ(material.textures.size(), 1U);
    EXPECT_EQ(material.textures.front().texture, AssetId{2});
}

TEST(Material, PacksTheDefaultsWithTheValuesItSets) {
    const CompiledShader shader = makeShader();
    Material material;
    material.set("tint", glm::vec4{0.5F, 0.25F, 1.0F, 1.0F});
    const std::vector<std::byte> block = packMaterialValues(shader, material);
    ASSERT_EQ(block.size(), shader.materialSize);
    EXPECT_FLOAT_EQ(floatAt(block, 0), 2.0F);  // speed keeps its default
    EXPECT_FLOAT_EQ(floatAt(block, 16), 0.5F);
    EXPECT_FLOAT_EQ(floatAt(block, 20), 0.25F);
    std::int32_t frames = 0;
    std::memcpy(&frames, &block.at(32), sizeof(frames));
    EXPECT_EQ(frames, 4);
}

TEST(Material, RejectsAnUnknownValueAnInstanceValueOrTheWrongType) {
    const CompiledShader shader = makeShader();
    EXPECT_THROW(static_cast<void>(packMaterialValues(shader, Material{}.set("missing", 1.0F))), ShaderException);
    EXPECT_THROW(static_cast<void>(packMaterialValues(shader, Material{}.set("flash", 1.0F))), ShaderException);
    EXPECT_THROW(static_cast<void>(packMaterialValues(shader, Material{}.set("speed", glm::vec2{1.0F}))),
                 ShaderException);
    EXPECT_THROW(static_cast<void>(packMaterialValues(shader, Material{}.set("frames", 4.0F))), ShaderException);
}

TEST(Material, ResolvesItsTexturesBySlot) {
    const CompiledShader shader = makeShader();
    Material material;
    material.setTexture("palette", AssetId{7});
    const auto slots = resolveMaterialTextures(shader, material);
    EXPECT_EQ(slots.at(0), AssetId{});
    EXPECT_EQ(slots.at(1), AssetId{7});
    EXPECT_THROW(static_cast<void>(resolveMaterialTextures(shader, Material{}.setTexture("missing", AssetId{1}))),
                 ShaderException);
}

TEST(MaterialAssets, AddsAMaterialAndReportsItOnce) {
    MaterialAssets materials;
    const AssetId id = materials.add("hit", Material{.shader = AssetId{3}});
    EXPECT_EQ(id, AssetId::fromPath("hit"));
    ASSERT_NE(materials.get(id), nullptr);
    EXPECT_EQ(materials.get(id)->shader, AssetId{3});
    EXPECT_EQ(materials.takeChanged(), (std::vector<AssetId>{id}));
    EXPECT_TRUE(materials.takeChanged().empty());
}

TEST(MaterialAssets, ChangingAValueReportsTheMaterialAgain) {
    MaterialAssets materials;
    const AssetId id = materials.add("hit", Material{.shader = AssetId{3}});
    static_cast<void>(materials.takeChanged());
    EXPECT_TRUE(materials.setValue(id, "speed", 4.0F));
    EXPECT_TRUE(materials.setTexture(id, "noise", AssetId{9}));
    EXPECT_EQ(materials.takeChanged(), (std::vector<AssetId>{id}));
    EXPECT_EQ(std::get<float>(materials.get(id)->values.front().value), 4.0F);
    EXPECT_FALSE(materials.setValue(AssetId::fromPath("missing"), "speed", 1.0F));
}

TEST(MaterialAssets, UnloadForgetsAMaterialAndReportsIt) {
    MaterialAssets materials;
    const AssetId id = materials.add("hit", Material{});
    EXPECT_TRUE(materials.unload(id));
    EXPECT_TRUE(materials.takeChanged().empty());  // never created, nothing to update
    EXPECT_EQ(materials.takeUnloaded(), (std::vector<AssetId>{id}));
    EXPECT_FALSE(materials.unload(id));
}

TEST(MaterialInstance, StartsFromTheShadersDefaults) {
    const CompiledShader shader = rtype::plugins::shader::detail::compileShader(
        "instance uniform float flash = 0.5;\nvoid fragment() { COLOR *= flash; }", "fx",
        std::array{rtype::contract::renderer::ShaderTarget::kSprite});
    EXPECT_FLOAT_EQ(MaterialInstance::of(shader).values[0].x, 0.5F);
}

TEST(MaterialInstance, SetsAnInstanceValueByName) {
    const CompiledShader shader = rtype::plugins::shader::detail::compileShader(
        "instance uniform float flash;\ninstance uniform vec2 offset;\nvoid fragment() { COLOR *= flash + offset.x; }",
        "fx", std::array{rtype::contract::renderer::ShaderTarget::kSprite});
    const MaterialInstance instance =
        MaterialInstance::of(shader).set(shader, "flash", 1.0F).set(shader, "offset", glm::vec2{2.0F, 3.0F});
    const auto offset =
        std::ranges::find(shader.instanceValues, "offset", &rtype::contract::renderer::ShaderValue::name)->offset;
    EXPECT_FLOAT_EQ(instance.values[0].x, 1.0F);
    EXPECT_FLOAT_EQ(instance.values.at(offset / 16)[static_cast<int>((offset % 16) / 4)], 2.0F);
}

TEST(MaterialInstance, RejectsAMaterialValueAnUnknownNameOrTheWrongType) {
    const CompiledShader shader = makeShader();
    MaterialInstance instance = MaterialInstance::of(shader);
    EXPECT_THROW(instance.set(shader, "speed", 1.0F), ShaderException);
    EXPECT_THROW(instance.set(shader, "missing", 1.0F), ShaderException);
    EXPECT_THROW(instance.set(shader, "flash", glm::vec4{1.0F}), ShaderException);
}
