/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapeBatchTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float4.hpp>

#include "plugins/backend/graphics/Shader.hpp"
#include "render/vulkan/rendering/ShapeBatch.hpp"

using rtype::render::vulkan::rendering::ShapeBatch;

namespace {
using MaterialId = rtype::plugins::backend::graphics::MaterialId;
using ShaderInstanceValues = rtype::plugins::backend::graphics::ShaderInstanceValues;

constexpr MaterialId kGlow{.index = 0, .generation = 1};
constexpr MaterialId kFlash{.index = 1, .generation = 1};
}  // namespace

TEST(ShapeBatch, MergesConsecutiveShapesOfTheDefaultMaterial) {
    ShapeBatch batch;
    batch.add({});
    batch.add({});
    ASSERT_EQ(batch.getBatches().size(), 1U);
    EXPECT_EQ(batch.getBatches()[0].material, MaterialId{});
    EXPECT_EQ(batch.getBatches()[0].instanceCount, 2U);
    EXPECT_EQ(batch.getInstances().size(), 2U);
    EXPECT_TRUE(batch.getInstanceValues().empty());
}

TEST(ShapeBatch, SplitsTheBatchesByMaterialAndKeepsTheDrawOrder) {
    ShapeBatch batch;
    batch.add({});
    batch.add(kGlow, {}, {});
    batch.add(kGlow, {}, {});
    batch.add(kFlash, {}, {});
    batch.add({});
    ASSERT_EQ(batch.getBatches().size(), 4U);
    EXPECT_EQ(batch.getBatches()[1].material, kGlow);
    EXPECT_EQ(batch.getBatches()[1].firstInstance, 1U);
    EXPECT_EQ(batch.getBatches()[1].instanceCount, 2U);
    EXPECT_EQ(batch.getBatches()[2].material, kFlash);
    EXPECT_EQ(batch.getBatches()[2].firstInstance, 3U);
    EXPECT_EQ(batch.getBatches()[3].material, MaterialId{});
}

TEST(ShapeBatch, KeepsTheInstanceValuesOfTheMaterialShapesOnly) {
    ShaderInstanceValues first{};
    first.front() = glm::vec4{1.0F};
    ShaderInstanceValues second{};
    second.front() = glm::vec4{2.0F};
    ShapeBatch batch;
    batch.add({});
    batch.add(kGlow, {}, first);
    batch.add({});
    batch.add(kFlash, {}, second);
    ASSERT_EQ(batch.getInstanceValues().size(), 2U);
    EXPECT_EQ(batch.getInstanceValues()[0], first);
    EXPECT_EQ(batch.getInstanceValues()[1], second);
    EXPECT_EQ(batch.getBatches()[1].firstValues, 0U);
    EXPECT_EQ(batch.getBatches()[3].firstValues, 1U);
}

TEST(ShapeBatch, TakesTheDefaultMaterialWithoutValues) {
    ShapeBatch batch;
    batch.add(MaterialId{}, {}, {});
    batch.add({});
    EXPECT_EQ(batch.getBatches().size(), 1U);
    EXPECT_TRUE(batch.getInstanceValues().empty());
}

TEST(ShapeBatch, ClearForgetsEverything) {
    ShapeBatch batch;
    batch.add(kGlow, {}, {});
    batch.clear();
    EXPECT_TRUE(batch.getInstances().empty());
    EXPECT_TRUE(batch.getInstanceValues().empty());
    EXPECT_TRUE(batch.getBatches().empty());
}
