/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicRendering
*/

#include "DynamicRendering.hpp"

#include <cstdint>
#include <span>
#include <vulkan/vulkan_raii.hpp>

namespace rtype::render::vulkan::rendering {

vk::RenderingAttachmentInfo colorAttachment(vk::ImageView view, const vk::ClearColorValue& clearColor) {
    return vk::RenderingAttachmentInfo{}
        .setImageView(view)
        .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setClearValue(clearColor);
}

vk::RenderingAttachmentInfo depthAttachment(vk::ImageView view, float clearDepth) {
    return vk::RenderingAttachmentInfo{}
        .setImageView(view)
        .setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setClearValue(vk::ClearDepthStencilValue{clearDepth, 0});
}

vk::RenderingAttachmentInfo withResolve(vk::RenderingAttachmentInfo attachment, vk::ImageView resolveView) {
    return attachment.setResolveMode(vk::ResolveModeFlagBits::eAverage)
        .setResolveImageView(resolveView)
        .setResolveImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare);
}

vk::RenderingInfo renderingInfo(vk::Extent2D extent, std::span<const vk::RenderingAttachmentInfo> colors,
                                const vk::RenderingAttachmentInfo* depth) {
    return vk::RenderingInfo{}
        .setRenderArea(vk::Rect2D{vk::Offset2D{0, 0}, extent})
        .setLayerCount(1)
        .setColorAttachmentCount(static_cast<std::uint32_t>(colors.size()))
        .setPColorAttachments(colors.data())
        .setPDepthAttachment(depth);
}

vk::PipelineRenderingCreateInfo pipelineRenderingInfo(std::span<const vk::Format> colorFormats,
                                                      vk::Format depthFormat) {
    return vk::PipelineRenderingCreateInfo{}
        .setColorAttachmentCount(static_cast<std::uint32_t>(colorFormats.size()))
        .setPColorAttachmentFormats(colorFormats.data())
        .setDepthAttachmentFormat(depthFormat);
}

void transitionImage(const vk::raii::CommandBuffer& commandBuffer, vk::Image image, const ImageTransition& transition) {
    const vk::ImageMemoryBarrier2 barrier =
        vk::ImageMemoryBarrier2{}
            .setSrcStageMask(transition.srcStage)
            .setSrcAccessMask(transition.srcAccess)
            .setDstStageMask(transition.dstStage)
            .setDstAccessMask(transition.dstAccess)
            .setOldLayout(transition.oldLayout)
            .setNewLayout(transition.newLayout)
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setImage(image)
            .setSubresourceRange(
                vk::ImageSubresourceRange{transition.aspect, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers});
    commandBuffer.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
}

}  // namespace rtype::render::vulkan::rendering
