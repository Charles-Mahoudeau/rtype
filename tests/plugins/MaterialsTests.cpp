/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MaterialsTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float4.hpp>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/Materials.hpp"
#include "plugins/backend/graphics/Shader.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::core::asset::AssetId;
using rtype::plugins::backend::graphics::MaterialId;
using rtype::plugins::backend::graphics::Materials;

static_assert(rtype::ecs::Resource<Materials>);

namespace {
constexpr MaterialId kFirst{.index = 3, .generation = 1};   ///< A material created in the renderer.
constexpr MaterialId kSecond{.index = 4, .generation = 1};  ///< Another one.
}  // namespace

TEST(Materials, StartsEmpty) {
    const Materials materials;
    EXPECT_EQ(materials.getCount(), 0U);
    EXPECT_EQ(materials.find(AssetId::fromPath("red")), nullptr);
}

TEST(Materials, FindsAMaterialSet) {
    Materials materials;
    Materials::Entry entry{.material = kFirst};
    entry.instanceDefaults.at(0) = glm::vec4{0.5F};
    materials.set(AssetId::fromPath("red"), entry);
    const Materials::Entry* found = materials.find(AssetId::fromPath("red"));
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->material, kFirst);
    EXPECT_EQ(found->instanceDefaults.at(0), glm::vec4{0.5F});
    EXPECT_EQ(materials.find(AssetId::fromPath("blue")), nullptr);
}

TEST(Materials, ReplacesWhatAnAssetHad) {
    Materials materials;
    materials.set(AssetId::fromPath("red"), Materials::Entry{.material = kFirst});
    materials.set(AssetId::fromPath("red"), Materials::Entry{.material = kSecond});
    EXPECT_EQ(materials.getCount(), 1U);
    EXPECT_EQ(materials.find(AssetId::fromPath("red"))->material, kSecond);
}

TEST(Materials, ForgetsARemovedAsset) {
    Materials materials;
    materials.set(AssetId::fromPath("red"), Materials::Entry{.material = kFirst});
    materials.set(AssetId::fromPath("blue"), Materials::Entry{.material = kSecond});
    materials.remove(AssetId::fromPath("red"));
    materials.remove(AssetId::fromPath("missing"));  // nothing to forget
    EXPECT_EQ(materials.find(AssetId::fromPath("red")), nullptr);
    EXPECT_NE(materials.find(AssetId::fromPath("blue")), nullptr);
    EXPECT_EQ(materials.getCount(), 1U);
}
