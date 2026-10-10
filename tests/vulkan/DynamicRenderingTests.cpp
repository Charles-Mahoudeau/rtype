/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicRenderingTests
*/

#include <gtest/gtest.h>

#include <array>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/rendering/DynamicRendering.hpp"

namespace rendering = rtype::render::vulkan::rendering;

TEST(DynamicRendering, ColorAttachmentClearsAndStores) {
    const vk::ClearColorValue clear{std::array{0.1F, 0.2F, 0.3F, 1.0F}};
    const vk::RenderingAttachmentInfo color = rendering::colorAttachment(vk::ImageView{}, clear);
    EXPECT_EQ(color.imageLayout, vk::ImageLayout::eColorAttachmentOptimal);
    EXPECT_EQ(color.loadOp, vk::AttachmentLoadOp::eClear);
    EXPECT_EQ(color.storeOp, vk::AttachmentStoreOp::eStore);
    EXPECT_EQ(color.resolveMode, vk::ResolveModeFlagBits::eNone);
    EXPECT_FLOAT_EQ(color.clearValue.color.float32[2], 0.3F);
}

TEST(DynamicRendering, DepthAttachmentClearsAndDiscards) {
    const vk::RenderingAttachmentInfo depth = rendering::depthAttachment(vk::ImageView{});
    EXPECT_EQ(depth.imageLayout, vk::ImageLayout::eDepthAttachmentOptimal);
    EXPECT_EQ(depth.loadOp, vk::AttachmentLoadOp::eClear);
    EXPECT_EQ(depth.storeOp, vk::AttachmentStoreOp::eDontCare);
    EXPECT_FLOAT_EQ(depth.clearValue.depthStencil.depth, 1.0F);
}

TEST(DynamicRendering, ResolveAveragesAndDiscardsTheMultisampledImage) {
    const vk::RenderingAttachmentInfo color =
        rendering::withResolve(rendering::colorAttachment(vk::ImageView{}, vk::ClearColorValue{}), vk::ImageView{});
    EXPECT_EQ(color.resolveMode, vk::ResolveModeFlagBits::eAverage);
    EXPECT_EQ(color.resolveImageLayout, vk::ImageLayout::eColorAttachmentOptimal);
    EXPECT_EQ(color.loadOp, vk::AttachmentLoadOp::eClear);
    EXPECT_EQ(color.storeOp, vk::AttachmentStoreOp::eDontCare);
}

TEST(DynamicRendering, RenderingInfoCoversTheWholeExtent) {
    const std::array colors{rendering::colorAttachment(vk::ImageView{}, vk::ClearColorValue{})};
    const vk::RenderingAttachmentInfo depth = rendering::depthAttachment(vk::ImageView{});
    const vk::RenderingInfo info = rendering::renderingInfo(vk::Extent2D{800, 600}, colors, &depth);
    EXPECT_EQ(info.renderArea.offset, (vk::Offset2D{0, 0}));
    EXPECT_EQ(info.renderArea.extent, (vk::Extent2D{800, 600}));
    EXPECT_EQ(info.layerCount, 1U);
    EXPECT_EQ(info.colorAttachmentCount, 1U);
    EXPECT_EQ(info.pColorAttachments, colors.data());
    EXPECT_EQ(info.pDepthAttachment, &depth);
}

TEST(DynamicRendering, PipelineRenderingInfoListsTheFormats) {
    const std::array formats{vk::Format::eB8G8R8A8Srgb};
    const vk::PipelineRenderingCreateInfo info = rendering::pipelineRenderingInfo(formats, vk::Format::eD32Sfloat);
    EXPECT_EQ(info.colorAttachmentCount, 1U);
    EXPECT_EQ(info.pColorAttachmentFormats, formats.data());
    EXPECT_EQ(info.depthAttachmentFormat, vk::Format::eD32Sfloat);
}

TEST(DynamicRendering, PresetTransitionsMatchTheFrame) {
    EXPECT_EQ(rendering::kToColorAttachment.oldLayout, vk::ImageLayout::eUndefined);
    EXPECT_EQ(rendering::kToColorAttachment.newLayout, vk::ImageLayout::eColorAttachmentOptimal);
    EXPECT_EQ(rendering::kColorAttachmentToPresent.oldLayout, vk::ImageLayout::eColorAttachmentOptimal);
    EXPECT_EQ(rendering::kColorAttachmentToPresent.newLayout, vk::ImageLayout::ePresentSrcKHR);
}
