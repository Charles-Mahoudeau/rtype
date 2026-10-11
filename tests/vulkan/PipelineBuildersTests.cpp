/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PipelineBuildersTests
*/

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/descriptor/DescriptorSetLayout.hpp"
#include "render/vulkan/descriptor/DescriptorWriter.hpp"
#include "render/vulkan/pipeline/GraphicsPipeline.hpp"
#include "render/vulkan/pipeline/Vertex2D.hpp"

namespace {

using rtype::render::vulkan::descriptor::DescriptorSetLayout;
using rtype::render::vulkan::descriptor::DescriptorWriter;
using rtype::render::vulkan::pipeline::GraphicsPipeline;

}  // namespace

TEST(GraphicsPipelineBuilder, IsIncompleteWithoutShadersFormatAndLayout) {
    GraphicsPipeline::Builder builder;
    EXPECT_FALSE(builder.isComplete());
    builder.setColorFormat(vk::Format::eB8G8R8A8Srgb).setBlend(GraphicsPipeline::BlendMode::kAlpha);
    EXPECT_FALSE(builder.isComplete());  // Still no shaders nor layout.
}

TEST(DescriptorSetLayoutBuilder, KeepsBindingsInOrder) {
    DescriptorSetLayout::Builder builder;
    builder.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
        .addBinding(3, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment, 4);
    ASSERT_EQ(builder.getBindings().size(), 2U);
    EXPECT_EQ(builder.getBindings()[1].binding, 3U);
    EXPECT_EQ(builder.getBindings()[1].descriptorType, vk::DescriptorType::eCombinedImageSampler);
    EXPECT_EQ(builder.getBindings()[1].descriptorCount, 4U);
    EXPECT_EQ(builder.getBindings()[1].stageFlags, vk::ShaderStageFlags{vk::ShaderStageFlagBits::eFragment});
}

TEST(DescriptorSetLayoutBuilder, RejectsDuplicateAndEmptyBindings) {
    DescriptorSetLayout::Builder builder;
    builder.addBinding(1, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex);
    EXPECT_THROW(builder.addBinding(1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute),
                 std::invalid_argument);
    EXPECT_THROW(builder.addBinding(2, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex, 0),
                 std::invalid_argument);
}

TEST(DescriptorWriter, BuildsOneWritePerBindingPointingToItsInfo) {
    DescriptorWriter writer;
    writer.writeBuffer(0, vk::Buffer{}, 16, 64)
        .writeImage(1, vk::ImageView{}, vk::Sampler{}, vk::ImageLayout::eShaderReadOnlyOptimal)
        .writeBuffer(2, vk::Buffer{}, 0, 32, vk::DescriptorType::eStorageBuffer);

    const std::vector<vk::WriteDescriptorSet> writes = writer.getWrites(vk::DescriptorSet{});
    ASSERT_EQ(writes.size(), 3U);
    EXPECT_EQ(writes[0].dstBinding, 0U);
    EXPECT_EQ(writes[0].descriptorType, vk::DescriptorType::eUniformBuffer);
    ASSERT_NE(writes[0].pBufferInfo, nullptr);
    EXPECT_EQ(writes[0].pBufferInfo->offset, 16U);
    EXPECT_EQ(writes[0].pBufferInfo->range, 64U);
    EXPECT_EQ(writes[1].descriptorType, vk::DescriptorType::eCombinedImageSampler);
    ASSERT_NE(writes[1].pImageInfo, nullptr);
    EXPECT_EQ(writes[1].pImageInfo->imageLayout, vk::ImageLayout::eShaderReadOnlyOptimal);
    EXPECT_EQ(writes[2].descriptorType, vk::DescriptorType::eStorageBuffer);
    EXPECT_EQ(writes[2].pBufferInfo->range, 32U);
}

TEST(DescriptorWriter, ClearForgetsEverything) {
    DescriptorWriter writer;
    writer.writeBuffer(0, vk::Buffer{}, 0, 4);
    writer.clear();
    EXPECT_TRUE(writer.getWrites(vk::DescriptorSet{}).empty());
}

TEST(Vertex2D, DescribesATightlyPackedVertex) {
    using rtype::render::vulkan::pipeline::Vertex2D;
    const auto bindings = Vertex2D::getBindings(1);
    EXPECT_EQ(bindings[0].binding, 1U);
    EXPECT_EQ(bindings[0].stride, 20U);
    EXPECT_EQ(bindings[0].inputRate, vk::VertexInputRate::eVertex);

    const auto attributes = Vertex2D::getAttributes(1);
    EXPECT_EQ(attributes[0].location, 0U);
    EXPECT_EQ(attributes[0].format, vk::Format::eR32G32Sfloat);
    EXPECT_EQ(attributes[0].offset, 0U);
    EXPECT_EQ(attributes[1].location, 1U);
    EXPECT_EQ(attributes[1].offset, 8U);
    EXPECT_EQ(attributes[2].location, 2U);
    EXPECT_EQ(attributes[2].format, vk::Format::eR8G8B8A8Unorm);  // Read as a vec4 of 0-1 floats.
    EXPECT_EQ(attributes[2].offset, 16U);
    for (const vk::VertexInputAttributeDescription& attribute : attributes) {
        EXPECT_EQ(attribute.binding, 1U);
    }
}
