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
#include <optional>
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
    return std::ranges::all_of(PhysicalDevice::kRequiredExtensions, [&availableExtensions](const char* name) {
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

/// @return The extensions the logical device must enable: the required ones, plus portability_subset when exposed.
std::vector<const char*> listDeviceExtensions(const vk::raii::PhysicalDevice& physicalDevice) {
    std::vector<const char*> extensions{PhysicalDevice::kRequiredExtensions.begin(),
                                        PhysicalDevice::kRequiredExtensions.end()};
    if (isExtensionAvailable(physicalDevice.enumerateDeviceExtensionProperties(),
                             PhysicalDevice::kPortabilitySubsetExtension)) {
        extensions.push_back(PhysicalDevice::kPortabilitySubsetExtension);
    }
    return extensions;
}

/// @brief Queue families found while searching a device; any may be missing.
struct QueueFamilyIndices {
    std::optional<std::uint32_t> graphicsFamily;  ///< Family that supports graphics commands.
    std::optional<std::uint32_t> presentFamily;   ///< Family that can present to the surface.
    std::optional<std::uint32_t> computeFamily;   ///< Family that supports compute, dedicated if possible.
    std::optional<std::uint32_t> transferFamily;  ///< Family for copies, dedicated if possible.

    /// @return Whether every family was found.
    [[nodiscard]] bool isComplete() const noexcept {
        return graphicsFamily && presentFamily && computeFamily && transferFamily;
    }
};

/// @return The first family whose flags contain all of @p wanted and none of @p excluded, if any.
std::optional<std::uint32_t> findFamily(const std::vector<vk::QueueFamilyProperties>& queueFamilies,
                                        vk::QueueFlags wanted, vk::QueueFlags excluded = {}) {
    std::uint32_t index = 0;
    for (const vk::QueueFamilyProperties& queueFamily : queueFamilies) {
        if ((queueFamily.queueFlags & wanted) == wanted && !(queueFamily.queueFlags & excluded)) {
            return index;
        }
        ++index;
    }
    return std::nullopt;
}

/// @return The first family able to present to @p surface, if any.
std::optional<std::uint32_t> findPresentFamily(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface,
                                               std::uint32_t familyCount) {
    for (std::uint32_t index = 0; index < familyCount; ++index) {
        if (physicalDevice.getSurfaceSupportKHR(index, surface) == VK_TRUE) {
            return index;
        }
    }
    return std::nullopt;
}

/// @return The queue families of @p physicalDevice. Compute and transfer prefer dedicated families, so their work
/// can run in parallel with graphics; otherwise compute takes any compute family, and transfer the graphics one
/// (graphics families implicitly support transfer).
QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& physicalDevice, const vk::SurfaceKHR& surface) {
    using Flag = vk::QueueFlagBits;
    const std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
    QueueFamilyIndices indices;

    indices.graphicsFamily = findFamily(queueFamilies, Flag::eGraphics);
    indices.presentFamily =
        findPresentFamily(physicalDevice, surface, static_cast<std::uint32_t>(queueFamilies.size()));
    indices.computeFamily = findFamily(queueFamilies, Flag::eCompute, Flag::eGraphics);
    if (!indices.computeFamily) {
        indices.computeFamily = findFamily(queueFamilies, Flag::eCompute);
    }
    indices.transferFamily = findFamily(queueFamilies, Flag::eTransfer, Flag::eGraphics | Flag::eCompute);
    if (!indices.transferFamily) {
        indices.transferFamily = indices.graphicsFamily;
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
    _deviceExtensions = listDeviceExtensions(_physicalDevice);

    const QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, surface);
    if (!indices.graphicsFamily || !indices.presentFamily || !indices.computeFamily || !indices.transferFamily) {
        throw std::runtime_error("Chosen GPU lacks a graphics, present, compute or transfer queue family");
    }
    _queueFamilies = QueueFamilies{
        .graphics = *indices.graphicsFamily,
        .present = *indices.presentFamily,
        .compute = *indices.computeFamily,
        .transfer = *indices.transferFamily,
    };
}
}  // namespace rtype::render::vulkan::core
