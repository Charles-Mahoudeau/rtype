/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Device
*/

#include "Device.hpp"

#include <cstdint>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "PhysicalDevice.hpp"

namespace rtype::render::vulkan::core {

namespace {

/// @brief Features enabled on the device, chained through pNext: core (1.0), Vulkan 1.1, 1.2 and 1.3.
using FeatureChain = vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan11Features,
                                        vk::PhysicalDeviceVulkan12Features, vk::PhysicalDeviceVulkan13Features>;

/// @brief Priority of every queue. Static, so it outlives the create infos that point to it.
constexpr float kQueuePriority = 1.0F;

/// @return VK_TRUE if @p enabled, VK_FALSE otherwise.
vk::Bool32 toBool32(bool enabled) noexcept { return enabled ? VK_TRUE : VK_FALSE; }

/// @return One queue create info per unique family: roles often share a family, and Vulkan forbids requesting the
/// same family twice.
std::vector<vk::DeviceQueueCreateInfo> makeQueueCreateInfos(const PhysicalDevice::QueueFamilies& families) {
    const std::set<std::uint32_t> uniqueFamilies{families.graphics, families.present, families.compute,
                                                 families.transfer};
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(uniqueFamilies.size());
    for (const std::uint32_t family : uniqueFamilies) {
        queueCreateInfos.push_back(
            vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex(family).setQueuePriorities(kQueuePriority));
    }
    return queueCreateInfos;
}

/// @return The features to enable: the required ones (dynamicRendering, synchronization2) always, the optional ones
/// only when @p optional says the physical device supports them.
FeatureChain makeFeatureChain(const PhysicalDevice::OptionalFeatures& optional) {
    return FeatureChain{
        vk::PhysicalDeviceFeatures2{}.setFeatures(
            vk::PhysicalDeviceFeatures{}.setSamplerAnisotropy(toBool32(optional.samplerAnisotropy))),
        vk::PhysicalDeviceVulkan11Features{},
        vk::PhysicalDeviceVulkan12Features{}
            .setTimelineSemaphore(toBool32(optional.timelineSemaphore))
            .setBufferDeviceAddress(toBool32(optional.bufferDeviceAddress))
            .setDescriptorIndexing(toBool32(optional.descriptorIndexing)),
        vk::PhysicalDeviceVulkan13Features{}.setDynamicRendering(VK_TRUE).setSynchronization2(VK_TRUE),
    };
}

}  // namespace

Device::Device(const PhysicalDevice& physicalDevice) {
    if (physicalDevice.getPhysicalDevice() == nullptr) {
        throw std::runtime_error("Physical device not selected");
    }

    const PhysicalDevice::QueueFamilies& families = physicalDevice.getQueueFamilies();
    const std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = makeQueueCreateInfos(families);
    const FeatureChain features = makeFeatureChain(physicalDevice.getOptionalFeatures());
    const vk::DeviceCreateInfo createInfo = vk::DeviceCreateInfo{}
                                                .setPNext(&features.get<vk::PhysicalDeviceFeatures2>())
                                                .setQueueCreateInfos(queueCreateInfos)
                                                .setPEnabledExtensionNames(physicalDevice.getDeviceExtensions());

    _device = vk::raii::Device{physicalDevice.getPhysicalDevice(), createInfo};
    _graphicsQueue = Queue{.handle = _device.getQueue(families.graphics, 0), .family = families.graphics};
    _presentQueue = Queue{.handle = _device.getQueue(families.present, 0), .family = families.present};
    _computeQueue = Queue{.handle = _device.getQueue(families.compute, 0), .family = families.compute};
    _transferQueue = Queue{.handle = _device.getQueue(families.transfer, 0), .family = families.transfer};
}

}  // namespace rtype::render::vulkan::core
