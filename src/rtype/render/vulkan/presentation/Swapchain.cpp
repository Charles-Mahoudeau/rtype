/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Swapchain
*/

#include "Swapchain.hpp"

#include <array>
#include <glm/ext/vector_uint2.hpp>
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

Swapchain::Swapchain(const glm::uvec2 framebufferSize, const core::PhysicalDevice& physicalDevice,
                     const vk::raii::SurfaceKHR& surface, const core::Device& device)
    : _swapChain(nullptr) {
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
}
}  // namespace rtype::render::vulkan::presentation
