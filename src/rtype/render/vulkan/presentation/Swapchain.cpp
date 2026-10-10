/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Swapchain
*/

#include "Swapchain.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <limits>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/core/PhysicalDevice.hpp"

namespace rtype::render::vulkan::presentation {

namespace {

/// @return B8G8R8A8_SRGB + SRGB_NONLINEAR when available, the first format otherwise.
vk::SurfaceFormatKHR chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& available) {
    const auto preferred = std::ranges::find_if(available, [](const vk::SurfaceFormatKHR& format) {
        return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear;
    });
    return preferred != available.end() ? *preferred : available.front();
}

/// @return @p preferred when supported, FIFO otherwise (the only mode every surface supports).
vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& available, vk::PresentModeKHR preferred) {
    return std::ranges::find(available, preferred) != available.end() ? preferred : vk::PresentModeKHR::eFifo;
}

/// @return The surface's extent when it imposes one, the framebuffer size clamped to its limits otherwise.
vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, glm::uvec2 framebufferSize) {
    if (capabilities.currentExtent.width != std::numeric_limits<std::uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    return vk::Extent2D{
        std::clamp(framebufferSize.x, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(framebufferSize.y, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)};
}

/// @return One image more than the minimum, so the CPU does not wait for the driver; clamped to the maximum
/// (0 means no maximum).
std::uint32_t chooseImageCount(const vk::SurfaceCapabilitiesKHR& capabilities) {
    const std::uint32_t count = capabilities.minImageCount + 1;
    return capabilities.maxImageCount > 0 ? std::min(count, capabilities.maxImageCount) : count;
}

/// @return COLOR_ATTACHMENT, plus TRANSFER_DST when the surface supports it (only COLOR_ATTACHMENT is guaranteed).
/// @throws std::runtime_error If the surface does not even support COLOR_ATTACHMENT.
vk::ImageUsageFlags chooseImageUsage(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (!(capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eColorAttachment)) {
        throw std::runtime_error("Swapchain: the surface does not support color attachment images");
    }
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eColorAttachment;
    if (capabilities.supportedUsageFlags & vk::ImageUsageFlagBits::eTransferDst) {
        usage |= vk::ImageUsageFlagBits::eTransferDst;
    }
    return usage;
}

/// @return The first supported composite alpha mode, opaque first (the window is not see-through).
vk::CompositeAlphaFlagBitsKHR chooseCompositeAlpha(const vk::SurfaceCapabilitiesKHR& capabilities) {
    constexpr std::array kPreferred{vk::CompositeAlphaFlagBitsKHR::eOpaque, vk::CompositeAlphaFlagBitsKHR::eInherit,
                                    vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
                                    vk::CompositeAlphaFlagBitsKHR::ePostMultiplied};
    const auto* const supported = std::ranges::find_if(kPreferred, [&capabilities](auto mode) {
        return static_cast<bool>(capabilities.supportedCompositeAlpha & mode);
    });
    if (supported == kPreferred.end()) {
        throw std::runtime_error("Swapchain: the surface supports no composite alpha mode");
    }
    return *supported;
}

}  // namespace

Swapchain::Swapchain(const core::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface,
                     const core::Device& device, glm::uvec2 framebufferSize, vk::PresentModeKHR presentMode,
                     vk::SwapchainKHR oldSwapchain) {
    if (framebufferSize.x == 0 || framebufferSize.y == 0) {
        throw std::runtime_error("Swapchain: the framebuffer is empty (minimized window)");
    }
    const core::PhysicalDevice::SwapChainSupportDetails support =
        core::PhysicalDevice::querySwapChainSupport(physicalDevice.getPhysicalDevice(), surface);
    const vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(support.formats);
    _format = surfaceFormat.format;
    _colorSpace = surfaceFormat.colorSpace;
    _extent = chooseExtent(support.capabilities, framebufferSize);
    _presentMode = choosePresentMode(support.presentModes, presentMode);

    const core::PhysicalDevice::QueueFamilies& families = physicalDevice.getQueueFamilies();
    const std::array queueFamilies{families.graphics, families.present};
    const bool shared = families.graphics != families.present;

    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.setSurface(*surface)
        .setMinImageCount(chooseImageCount(support.capabilities))
        .setImageFormat(_format)
        .setImageColorSpace(_colorSpace)
        .setImageExtent(_extent)
        .setImageArrayLayers(1)
        .setImageUsage(chooseImageUsage(support.capabilities))
        .setImageSharingMode(shared ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive)
        .setPreTransform(support.capabilities.currentTransform)
        .setCompositeAlpha(chooseCompositeAlpha(support.capabilities))
        .setPresentMode(_presentMode)
        .setClipped(vk::True)
        .setOldSwapchain(oldSwapchain);
    if (shared) {
        createInfo.setQueueFamilyIndices(queueFamilies);
    }

    _swapchain = vk::raii::SwapchainKHR{device.getDevice(), createInfo};
    _images = _swapchain.getImages();
    createImageViews(device);
}

void Swapchain::createImageViews(const core::Device& device) {
    _imageViews.reserve(_images.size());
    for (const vk::Image image : _images) {
        const vk::ImageViewCreateInfo viewInfo =
            vk::ImageViewCreateInfo{}
                .setImage(image)
                .setViewType(vk::ImageViewType::e2D)
                .setFormat(_format)
                .setSubresourceRange(vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});
        _imageViews.emplace_back(device.getDevice(), viewInfo);
    }
}

}  // namespace rtype::render::vulkan::presentation
