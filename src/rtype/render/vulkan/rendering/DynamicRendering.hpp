/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicRendering
*/

#pragma once

#include <span>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"

/// @brief Helpers for dynamic rendering (vkCmdBeginRendering, core in Vulkan 1.3).
///
/// @details Dynamic rendering replaces VkRenderPass and VkFramebuffer: there is no object to create, the attachments
/// are described each frame when recording, and pipelines only declare their attachment formats. What a render pass
/// did implicitly, the image layout transitions, is done with transitionImage(). A frame typically records:
/// @code
/// transitionImage(cmd, image, kToColorAttachment);
/// const vk::RenderingAttachmentInfo color = colorAttachment(view, clearColor);
/// cmd.beginRendering(renderingInfo(extent, color));
/// // draws
/// cmd.endRendering();
/// transitionImage(cmd, image, kColorAttachmentToPresent);
/// @endcode
namespace rtype::render::vulkan::rendering {

/// @brief A layout transition of an image, with the stages and accesses it orders (Synchronization2).
struct ImageTransition {
    vk::ImageLayout oldLayout = vk::ImageLayout::eUndefined;               ///< Layout before.
    vk::ImageLayout newLayout = vk::ImageLayout::eUndefined;               ///< Layout after.
    vk::PipelineStageFlags2 srcStage = vk::PipelineStageFlagBits2::eNone;  ///< Stages that must finish first.
    vk::AccessFlags2 srcAccess = vk::AccessFlagBits2::eNone;               ///< Writes to make available.
    vk::PipelineStageFlags2 dstStage = vk::PipelineStageFlagBits2::eNone;  ///< Stages that wait for it.
    vk::AccessFlags2 dstAccess = vk::AccessFlagBits2::eNone;               ///< Accesses that wait for it.
    vk::ImageAspectFlags aspect = vk::ImageAspectFlagBits::eColor;         ///< Aspect of the image.
};

/// @brief Swapchain image, contents discarded, to color attachment: waits for the acquire semaphore's stage
/// (COLOR_ATTACHMENT_OUTPUT), before the attachment is written.
inline constexpr ImageTransition kToColorAttachment{
    .oldLayout = vk::ImageLayout::eUndefined,
    .newLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .srcStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .srcAccess = vk::AccessFlagBits2::eNone,
    .dstStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .dstAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
};

/// @brief Color attachment to presentable: after the attachment writes; presentation itself waits on a semaphore.
inline constexpr ImageTransition kColorAttachmentToPresent{
    .oldLayout = vk::ImageLayout::eColorAttachmentOptimal,
    .newLayout = vk::ImageLayout::ePresentSrcKHR,
    .srcStage = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    .srcAccess = vk::AccessFlagBits2::eColorAttachmentWrite,
    .dstStage = vk::PipelineStageFlagBits2::eNone,
    .dstAccess = vk::AccessFlagBits2::eNone,
};

/// @return A color attachment cleared to @p clearColor at the start of rendering and stored at the end
/// (CLEAR / STORE), in COLOR_ATTACHMENT_OPTIMAL.
[[nodiscard]] RTYPE_RENDER_VULKAN_API vk::RenderingAttachmentInfo colorAttachment(
    vk::ImageView view, const vk::ClearColorValue& clearColor);

/// @return A depth attachment cleared to @p clearDepth and discarded at the end (CLEAR / DONT_CARE: depth is only
/// needed while drawing), in DEPTH_ATTACHMENT_OPTIMAL.
[[nodiscard]] RTYPE_RENDER_VULKAN_API vk::RenderingAttachmentInfo depthAttachment(vk::ImageView view,
                                                                                  float clearDepth = 1.0F);

/// @return @p attachment, multisampled, resolved (averaged) into @p resolveView at the end of rendering. The
/// multisampled contents are then discarded (DONT_CARE): only the resolved image is kept.
[[nodiscard]] RTYPE_RENDER_VULKAN_API vk::RenderingAttachmentInfo withResolve(vk::RenderingAttachmentInfo attachment,
                                                                              vk::ImageView resolveView);

/// @return What vkCmdBeginRendering needs: the whole @p extent as render area, one layer, @p colors and the optional
/// @p depth. The result points to @p colors and @p depth: keep them alive until beginRendering().
[[nodiscard]] RTYPE_RENDER_VULKAN_API vk::RenderingInfo renderingInfo(
    vk::Extent2D extent, std::span<const vk::RenderingAttachmentInfo> colors,
    const vk::RenderingAttachmentInfo* depth = nullptr);

/// @return What a pipeline chains into its pNext instead of a render pass: the formats of its attachments. The result
/// points to @p colorFormats: keep it alive until the pipeline is created.
[[nodiscard]] RTYPE_RENDER_VULKAN_API vk::PipelineRenderingCreateInfo pipelineRenderingInfo(
    std::span<const vk::Format> colorFormats, vk::Format depthFormat = vk::Format::eUndefined);

/// @brief Records a pipeline barrier (vkCmdPipelineBarrier2) moving every mip level and layer of @p image from
/// @p transition's old layout to its new one.
RTYPE_RENDER_VULKAN_API void transitionImage(const vk::raii::CommandBuffer& commandBuffer, vk::Image image,
                                             const ImageTransition& transition);

}  // namespace rtype::render::vulkan::rendering
