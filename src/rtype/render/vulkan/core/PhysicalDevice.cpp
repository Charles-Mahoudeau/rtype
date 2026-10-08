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

namespace rtype::render::vulkan {

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

bool checkDeviceFeatures(const vk::raii::PhysicalDevice& physicalDevice) {
    const auto features =
        physicalDevice.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                                             vk::PhysicalDeviceVulkan13Features,
                                             vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
    const bool supportsRequiredFeatures =
        static_cast<bool>(features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters) &&
        static_cast<bool>(features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering) &&
        static_cast<bool>(
            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState);
    return supportsRequiredFeatures;
}

/// @return The first queue family supporting graphics and the first one able to present to @p surface.
PhysicalDevice::QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& physicalDevice,
                                                     const vk::SurfaceKHR& surface) {
    const std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
    PhysicalDevice::QueueFamilyIndices indices;

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

rtype::render::vulkan::PhysicalDevice::SwapChainSupportDetails querySwapChainSupport(
    const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) {
    rtype::render::vulkan::PhysicalDevice::SwapChainSupportDetails details;
    details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
    details.formats = device.getSurfaceFormatsKHR(surface);
    details.presentModes = device.getSurfacePresentModesKHR(surface);
    return details;
}

bool isDeviceSuitable(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface) {
    const bool queueFamiliesSupported = findQueueFamilies(device, surface).isComplete();
    if (!checkDeviceFeatures(device)) {
        return false;
    }
    const bool supportsVulkan1_3 = device.getProperties().apiVersion >= vk::ApiVersion13;
    const bool extensionsSupported = checkDeviceExtensionSupport(device);
    if (!extensionsSupported) {
        return false;
    }
    const rtype::render::vulkan::PhysicalDevice::SwapChainSupportDetails swapChainSupport =
        querySwapChainSupport(device, surface);
    const bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

    return supportsVulkan1_3 && extensionsSupported && swapChainAdequate && queueFamiliesSupported;
}

unsigned int rateDeviceSuitability(const vk::raii::PhysicalDevice& device, const vk::raii::SurfaceKHR& surface,
                                   vk::PhysicalDeviceType preferredType) {
    const vk::PhysicalDeviceProperties deviceProperties = device.getProperties();

    if (!isDeviceSuitable(device, surface)) {
        return 0;
    }

    unsigned int score = 0;
    if (deviceProperties.deviceType == preferredType) {
        score += 1000;
    }
    score += deviceProperties.limits.maxImageDimension2D;

    return score;
}
}  // namespace

void PhysicalDevice::pickPhysicalDevice(const vk::raii::SurfaceKHR& surface, vk::PhysicalDeviceType preferredType) {
    const auto deviceCount = static_cast<uint32_t>(_physicalDevices.size());

    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::multimap<unsigned int, vk::raii::PhysicalDevice> candidates;
    for (uint32_t i = 0; i < deviceCount; i++) {
        unsigned int score = rateDeviceSuitability(_physicalDevices[i], surface, preferredType);
        candidates.insert(std::make_pair(score, _physicalDevices[i]));
    }
    if (candidates.rbegin()->first > 0) {
        _physicalDevice = candidates.rbegin()->second;
    } else {
        throw std::runtime_error("Failed to find a suitable GPU");
    }
}

PhysicalDevice::PhysicalDevice(vk::raii::Instance& instance, const vk::raii::SurfaceKHR& surface,
                               vk::PhysicalDeviceType preferredType)
    : _physicalDevices(nullptr), _physicalDevice(nullptr) {
    _physicalDevices = vk::raii::PhysicalDevices(instance);
    if (_physicalDevices.empty()) {
        throw std::runtime_error("No Vulkan physical devices found");
    }
    pickPhysicalDevice(surface, preferredType);
    if (_physicalDevice == nullptr) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }
}
}  // namespace rtype::render::vulkan
