/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderHandlesTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <string_view>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderHandles.hpp"
#include "rtype/contract/renderer/shaders/CompiledShader.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::core::asset::AssetId;
using rtype::plugins::shader::Material;
using rtype::plugins::shader::ShaderHandles;

static_assert(rtype::ecs::Resource<ShaderHandles>);

namespace {
/// @brief A shader with a material value, a texture and an instance value.
constexpr std::string_view kShader =
    "uniform vec4 tint = vec4(1.0);\n"
    "uniform sampler2D noise;\n"
    "instance uniform float flash = 0.5;\n"
    "void fragment() { COLOR = mix(COLOR * tint, texture(noise, UV), flash); }\n";

/// @brief The asset resources, a fake renderer that supports shaders, and the handles under test.
class ShaderHandlesTest : public ::testing::Test {
  protected:
    void SetUp() override { _state->shaders = true; }

    /// @brief Runs ShaderHandles::update(), as the ShaderPlugin does each frame.
    void update() { _handles.update(_shaders, _materials, _textures, _renderer); }

    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state{
        std::make_shared<rtype::test::plugins::FakeBackendState>()};  ///< Drives the fake renderer.
    rtype::test::plugins::FakeRenderer _renderer{_state};             ///< Records the calls.
    rtype::plugins::shader::ShaderAssets _shaders;                    ///< The game's shaders.
    rtype::plugins::shader::MaterialAssets _materials;                ///< The game's materials.
    rtype::plugins::graphics::TextureAssets _textures;                ///< The game's textures.
    ShaderHandles _handles;                                           ///< Under test.
};
}  // namespace

TEST_F(ShaderHandlesTest, CreatesTheShadersAndTheirMaterials) {
    const AssetId shader = _shaders.add("glow", kShader);
    const AssetId material =
        _materials.add("red", Material{.shader = shader}.set("tint", glm::vec4{1.0F, 0.0F, 0.0F, 1.0F}));
    update();
    ASSERT_EQ(_state->createdShaders.size(), 1U);
    ASSERT_EQ(_state->createdMaterials.size(), 1U);
    EXPECT_TRUE(_handles.hasShader(shader));
    EXPECT_EQ(_state->createdMaterials[0].values.size(), _shaders.get(shader)->materialSize);
    const ShaderHandles::Material* created = _handles.getMaterial(material);
    ASSERT_NE(created, nullptr);
    EXPECT_NE(created->material, rtype::contract::renderer::MaterialId{});
    EXPECT_FLOAT_EQ(created->instanceDefaults[0].x, 0.5F);
}

TEST_F(ShaderHandlesTest, CreatesNothingWithoutShaderSupport) {
    _state->shaders = false;
    const AssetId shader = _shaders.add("glow", kShader);
    const AssetId material = _materials.add("red", Material{.shader = shader});
    update();
    EXPECT_TRUE(_state->createdShaders.empty());
    EXPECT_EQ(_handles.getMaterial(material), nullptr);
    EXPECT_TRUE(_shaders.takeAdded().empty());
    EXPECT_TRUE(_materials.takeChanged().empty());
}

TEST_F(ShaderHandlesTest, AMaterialWaitsForItsShader) {
    const AssetId material = _materials.add("red", Material{.shader = AssetId::fromPath("glow")});
    update();
    EXPECT_EQ(_handles.getMaterial(material), nullptr);
    static_cast<void>(_shaders.add("glow", kShader));
    update();
    EXPECT_NE(_handles.getMaterial(material), nullptr);
}

TEST_F(ShaderHandlesTest, AMaterialWaitsForItsTextures) {
    const AssetId noise = _textures.load("noise.png", rtype::test::plugins::twoPixelPng());
    const AssetId material =
        _materials.add("red", Material{.shader = _shaders.add("glow", kShader)}.setTexture("noise", noise));
    update();
    EXPECT_EQ(_handles.getMaterial(material), nullptr);
    rtype::plugins::graphics::updateTextures(_textures, _renderer);
    update();
    ASSERT_EQ(_state->createdMaterials.size(), 1U);
    EXPECT_EQ(_state->createdMaterials[0].textures[0], *_textures.getTexture(noise));
}

TEST_F(ShaderHandlesTest, UpdatesAChangedMaterial) {
    const AssetId material = _materials.add("red", Material{.shader = _shaders.add("glow", kShader)});
    update();
    ASSERT_TRUE(_materials.setValue(material, "tint", glm::vec4{0.5F}));
    update();
    EXPECT_EQ(_state->createdMaterials.size(), 1U);
    EXPECT_EQ(_state->updatedMaterials.size(), 1U);
}

TEST_F(ShaderHandlesTest, ReportsAMaterialItsShaderDoesNotMatchOnce) {
    const AssetId material =
        _materials.add("red", Material{.shader = _shaders.add("glow", kShader)}.set("missing", 1.0F));
    update();
    update();
    EXPECT_EQ(_handles.getMaterial(material), nullptr);
    EXPECT_TRUE(_state->createdMaterials.empty());
}

TEST_F(ShaderHandlesTest, DestroysWhatIsUnloaded) {
    const AssetId shader = _shaders.add("glow", kShader);
    const AssetId material = _materials.add("red", Material{.shader = shader});
    update();
    ASSERT_TRUE(_materials.unload(material));
    ASSERT_TRUE(_shaders.unload(shader));
    update();
    EXPECT_EQ(_state->destroyedMaterials.size(), 1U);
    EXPECT_EQ(_state->destroyedShaders.size(), 1U);
    EXPECT_EQ(_handles.getMaterial(material), nullptr);
    EXPECT_FALSE(_handles.hasShader(shader));
}

TEST_F(ShaderHandlesTest, RecreatesTheMaterialsOfAReloadedShader) {
    const AssetId shader = _shaders.add("glow", kShader);
    const AssetId material = _materials.add("red", Material{.shader = shader});
    update();
    ASSERT_TRUE(_shaders.unload(shader));
    update();
    EXPECT_EQ(_state->destroyedMaterials.size(), 1U);
    EXPECT_EQ(_handles.getMaterial(material), nullptr);
    static_cast<void>(_shaders.add("glow", kShader));
    update();
    EXPECT_EQ(_state->createdMaterials.size(), 2U);
    EXPECT_NE(_handles.getMaterial(material), nullptr);
}
