/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Device
*/

#include "Device.hpp"

#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "PhysicalDevice.hpp"

namespace rtype::render::vulkan::core {

namespace {

/// @brief Features enabled on the device, chained through pNext: core (1.0), Vulkan 1.2 and Vulkan 1.3.
using FeatureChain = vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan12Features,
                                        vk::PhysicalDeviceVulkan13Features>;

/// @brief Priority of every queue. Static, so it outlives the create infos that point to it.
constexpr float kQueuePriority = 1.0F;

/// @return VK_TRUE if @p enabled, VK_FALSE otherwise.
vk::Bool32 toBool32(bool enabled) noexcept { return enabled ? VK_TRUE : VK_FALSE; }

/// @return One queue create info per unique family: graphics and present often share the same one, and Vulkan
/// forbids requesting the same family twice.
std::vector<vk::DeviceQueueCreateInfo> makeQueueCreateInfos(const PhysicalDevice::QueueFamilies& families) {
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{
        vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex(families.graphics).setQueuePriorities(kQueuePriority),
    };
    if (families.present != families.graphics) {
        queueCreateInfos.push_back(
            vk::DeviceQueueCreateInfo{}.setQueueFamilyIndex(families.present).setQueuePriorities(kQueuePriority));
    }
    return queueCreateInfos;
}

/// @return The features to enable: the required ones (dynamicRendering, synchronization2) always, the optional ones
/// only when @p optional says the physical device supports them.
FeatureChain makeFeatureChain(const PhysicalDevice::OptionalFeatures& optional) {
    return FeatureChain{
        vk::PhysicalDeviceFeatures2{}.setFeatures(
            vk::PhysicalDeviceFeatures{}.setSamplerAnisotropy(toBool32(optional.samplerAnisotropy))),
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
                                                .setPEnabledExtensionNames(PhysicalDevice::kRequiredExtensions);

    _device = vk::raii::Device{physicalDevice.getPhysicalDevice(), createInfo};
    _graphicsQueue = _device.getQueue(families.graphics, 0);
    _presentQueue = _device.getQueue(families.present, 0);
}

}  // namespace rtype::render::vulkan::core
