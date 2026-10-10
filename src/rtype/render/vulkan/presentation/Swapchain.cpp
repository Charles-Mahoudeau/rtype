/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Swapchain
*/

#include "Swapchain.hpp"

#include <algorithm>
#include <array>
#include <glm/ext/vector_uint2.hpp>
#include <limits>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

#include "../core/Device.hpp"
#include "../core/PhysicalDevice.hpp"
namespace rtype::render::vulkan::presentation {

namespace {
vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == vk::Format::eB8G8R8A8Srgb &&
            availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
            return availablePresentMode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, const glm::uvec2& framebufferSize) {
    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    vk::Extent2D actualExtent = {framebufferSize.x, framebufferSize.y};
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    return actualExtent;
}

uint32_t chooseSwapMinImageCount(const core::PhysicalDevice::SwapChainSupportDetails& swapChainSupport) {
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    return imageCount;
}

vk::SharingMode chooseSwapSharingMode(const core::PhysicalDevice::QueueFamilies& queueFamilies) {
    if (queueFamilies.graphics != queueFamilies.present) {
        return vk::SharingMode::eConcurrent;
    }
    return vk::SharingMode::eExclusive;
}

uint32_t chooseQueueFamilyIndexCount(const core::PhysicalDevice::QueueFamilies& queueFamilies) {
    if (queueFamilies.graphics != queueFamilies.present) {
        return 2;
    }
    return 0;
}

}  // namespace

void Swapchain::createFramebuffers(const core::Device& device, const vk::Extent2D& swapChainExtent) {
    _framebuffers.clear();
    _framebuffers.reserve(_imageViews.size());

    for (const auto& imageView : _imageViews) {
        const vk::FramebufferCreateInfo framebufferInfo({}, *renderPass, imageView, swapChainExtent.width,
                                                        swapChainExtent.height, 1);

        _framebuffers.emplace_back(device.getDevice(), framebufferInfo);
    }
}

void Swapchain::createImageViews(const core::Device& device, const vk::Format swapChainImageFormat) {
    _imageViews.clear();
    std::vector<vk::Image> swapChainImages = _swapChain.getImages();
    _imageViews.reserve(_swapChain.getImages().size());

    for (const auto& image : swapChainImages) {
        const vk::ImageViewCreateInfo viewInfo({}, image, vk::ImageViewType::e2D, swapChainImageFormat,
                                               vk::ComponentMapping{},
                                               vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));

        _imageViews.emplace_back(device.getDevice(), viewInfo);
    }
}

vk::raii::RenderPass Swapchain::createRenderPass(const core::Device& device, const vk::Format swapChainImageFormat) {
    const vk::AttachmentDescription colorAttachment({}, swapChainImageFormat, vk::SampleCountFlagBits::e1,
                                                    vk::AttachmentLoadOp::eClear, vk::AttachmentStoreOp::eStore,
                                                    vk::AttachmentLoadOp::eDontCare, vk::AttachmentStoreOp::eDontCare,
                                                    vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR);

    const vk::AttachmentReference colorAttachmentRef(0, vk::ImageLayout::eColorAttachmentOptimal);

    const vk::SubpassDescription subpass({}, vk::PipelineBindPoint::eGraphics, 0, nullptr, 1, &colorAttachmentRef);

    const vk::SubpassDependency dependency(VK_SUBPASS_EXTERNAL, 0, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                           vk::PipelineStageFlagBits::eColorAttachmentOutput, {},
                                           vk::AccessFlagBits::eColorAttachmentWrite);

    const vk::RenderPassCreateInfo renderPassInfo({}, 1, &colorAttachment, 1, &subpass, 1, &dependency);
    return {device.getDevice(), renderPassInfo};
}

void Swapchain::recreate(const glm::uvec2 framebufferSize, const core::PhysicalDevice& physicalDevice,
                         const vk::raii::SurfaceKHR& surface, const core::Device& device) {
    _swapChain = nullptr;
    _imageViews.clear();
    _framebuffers.clear();
    *this = Swapchain(framebufferSize, physicalDevice, surface, device);
}

Swapchain::~Swapchain() {
    _framebuffers.clear();
    _imageViews.clear();
    _swapChain = nullptr;
    renderPass = nullptr;
}

Swapchain::Swapchain(const glm::uvec2 framebufferSize, const core::PhysicalDevice& physicalDevice,
                     const vk::raii::SurfaceKHR& surface, const core::Device& device)
    : _swapChain(nullptr), renderPass(nullptr) {
    core::PhysicalDevice::SwapChainSupportDetails swapChainSupport =
        core::PhysicalDevice::querySwapChainSupport(physicalDevice.getPhysicalDevice(), surface);

    const vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    const vk::PresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    const vk::Extent2D extent = chooseSwapExtent(swapChainSupport.capabilities, framebufferSize);
    const uint32_t minImageCount = chooseSwapMinImageCount(swapChainSupport);
    const uint32_t queueFamilyIndexCount = chooseQueueFamilyIndexCount(physicalDevice.getQueueFamilies());
    const std::array<uint32_t, 4> queueFamilyIndices = {
        physicalDevice.getQueueFamilies().graphics, physicalDevice.getQueueFamilies().present,
        physicalDevice.getQueueFamilies().compute, physicalDevice.getQueueFamilies().transfer};

    vk::SwapchainCreateInfoKHR swapChainCreateInfo{};
    swapChainCreateInfo.surface = *surface;
    swapChainCreateInfo.minImageCount = minImageCount;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc |
                                     vk::ImageUsageFlagBits::eTransferDst;
    swapChainCreateInfo.imageSharingMode = chooseSwapSharingMode(physicalDevice.getQueueFamilies());
    swapChainCreateInfo.queueFamilyIndexCount = queueFamilyIndexCount;
    swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndexCount > 0 ? queueFamilyIndices.data() : nullptr;
    swapChainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR(swapChainSupport.capabilities.currentTransform);
    swapChainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.clipped = vk::True;
    swapChainCreateInfo.oldSwapchain = nullptr;
    _swapChain = vk::raii::SwapchainKHR(device.getDevice(), swapChainCreateInfo);
    createImageViews(device, surfaceFormat.format);
    createFramebuffers(device, extent);
    renderPass = createRenderPass(device, surfaceFormat.format);
}
}  // namespace rtype::render::vulkan::presentation
