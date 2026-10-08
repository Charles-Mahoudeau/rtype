/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicalDevice
*/

#include "PhysicalDevice.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

namespace rtype::render::vulkan::core {

namespace {

bool isExtensionAvailable(const std::vector<vk::ExtensionProperties>& availableExtensions, std::string_view name) {
    return std::ranges::any_of(availableExtensions, [name](const vk::ExtensionProperties& extension) {
        return std::string_view{extension.extensionName} == name;
    });
}

bool checkDeviceExtensionSupport(const vk::raii::PhysicalDevice& physicalDevice) {
    const std::vector<vk::ExtensionProperties> availableExtensions =
        physicalDevice.enumerateDeviceExtensionProperties();
#ifdef __APPLE__
    const std::vector<const char*> requiredExtensions{vk::KHRSwapchainExtensionName, "VK_KHR_portability_subset"};
#else
    const std::vector<const char*> requiredExtensions{vk::KHRSwapchainExtensionName};
#endif

    return std::ranges::all_of(requiredExtensions, [&availableExtensions](const char* name) {
        return isExtensionAvailable(availableExtensions, name);
    });
}

/// @return Whether @p physicalDevice supports every feature the renderer needs: dynamicRendering and
/// synchronization2. Only valid on a Vulkan 1.3 device.
bool checkRequiredFeatures(const vk::raii::PhysicalDevice& physicalDevice) {
    const auto features =
        physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features>();
    const auto& vulkan13 = features.get<vk::PhysicalDeviceVulkan13Features>();
    return vulkan13.dynamicRendering == VK_TRUE && vulkan13.synchronization2 == VK_TRUE;
}

/// @return The optional features @p physicalDevice supports. Only valid on a Vulkan 1.2 device or newer.
PhysicalDevice::OptionalFeatures queryOptionalFeatures(const vk::raii::PhysicalDevice& physicalDevice) {
    const auto features =
        physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan12Features>();
    const vk::PhysicalDeviceFeatures& core = features.get<vk::PhysicalDeviceFeatures2>().features;
    const auto& vulkan12 = features.get<vk::PhysicalDeviceVulkan12Features>();
    return PhysicalDevice::OptionalFeatures{
        .timelineSemaphore = vulkan12.timelineSemaphore == VK_TRUE,
        .samplerAnisotropy = core.samplerAnisotropy == VK_TRUE,
        .bufferDeviceAddress = vulkan12.bufferDeviceAddress == VK_TRUE,
        .descriptorIndexing = vulkan12.descriptorIndexing == VK_TRUE,
    };
}

/// @return The limits of @p physicalDevice the renderer needs.
PhysicalDevice::Limits queryLimits(const vk::raii::PhysicalDevice& physicalDevice) {
    const vk::PhysicalDeviceLimits limits = physicalDevice.getProperties().limits;
    return PhysicalDevice::Limits{
        .minUniformBufferOffsetAlignment = limits.minUniformBufferOffsetAlignment,
        .maxPushConstantsSize = limits.maxPushConstantsSize,
        .framebufferColorSampleCounts = limits.framebufferColorSampleCounts,
        .timestampPeriod = limits.timestampPeriod,
    };
}

/// @return The first queue family supporting graphics and the first one able to present to @p surface.
core::PhysicalDevice::QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& physicalDevice,
                                                           const vk::SurfaceKHR& surface) {
    const std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
    core::PhysicalDevice::QueueFamilyIndices indices;

    std::uint32_t index = 0;
    for (const vk::QueueFamilyProperties& queueFamily : queueFamilies) {
        if (!indices.graphicsFamily && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
            indices.graphicsFamily = index;
        }
        if (!indices.presentFamily && physicalDevice.getSurfaceSupportKHR(index, surface) == VK_TRUE) {
            indices.presentFamily = index;
        }
        if (indices.isComplete()) {
            break;
        }
        ++index;
    }
    return indices;
}

core::PhysicalDevice::SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice& device,
                                                                    const vk::raii::SurfaceKHR& surface) {
    core::PhysicalDevice::SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
}

/// @details The API version is checked first: querying the Vulkan 1.3 features of an older device is invalid usage.
bool isDeviceSuitable(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) {
    if (device.getProperties().apiVersion < vk::ApiVersion13 || !checkRequiredFeatures(device) ||
        !checkDeviceExtensionSupport(device) || !findQueueFamilies(device, surface).isComplete()) {
        return false;
    }
    const core::PhysicalDevice::SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, surface);
    return !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
}

unsigned int rateDeviceSuitability(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface,
                                   vk::PhysicalDeviceType preferredType) {
    const vk::PhysicalDeviceProperties deviceProperties = device.getProperties();

    if (!isDeviceSuitable(device, surface)) {
        return 0;
    }

    constexpr unsigned int kPreferredBit = 1U << 31U;
    unsigned int score = std::min(deviceProperties.limits.maxImageDimension2D, kPreferredBit - 1U) + 1U;
    if (deviceProperties.deviceType == preferredType) {
        score |= kPreferredBit;
    }
    return score;
}
}  // namespace

void PhysicalDevice::pickPhysicalDevice(const vk::raii::SurfaceKHR& surface, vk::PhysicalDeviceType preferredType) {
    if (_physicalDevices.empty()) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::multimap<unsigned int, vk::raii::PhysicalDevice> candidates;
    for (const vk::raii::PhysicalDevice& physicalDevice : _physicalDevices) {
        candidates.emplace(rateDeviceSuitability(physicalDevice, surface, preferredType), physicalDevice);
    }
    if (candidates.rbegin()->first > 0) {
        _physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("Failed to find a suitable GPU");
    }
}

PhysicalDevice::PhysicalDevice(const Instance& instance, const vk::raii::SurfaceKHR& surface,
                               vk::PhysicalDeviceType preferredType)
    : _physicalDevices(nullptr), _physicalDevice(nullptr) {
    _physicalDevices = vk::raii::PhysicalDevices(instance.getInstance());
    if (_physicalDevices.empty()) {
        throw std::runtime_error("No Vulkan physical devices found");
    }
    pickPhysicalDevice(surface, preferredType);
    if (_physicalDevice == nullptr) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }
    _optionalFeatures = queryOptionalFeatures(_physicalDevice);
    _limits = queryLimits(_physicalDevice);
}
}  // namespace rtype::render::vulkan::core
