/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Device
*/

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "PhysicalDevice.hpp"
#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::core {

/// @brief The logical device created from the chosen PhysicalDevice, and its graphics and present queues.
/// @details The logical device is the handle used to create resources and submit commands to the GPU. It enables the
/// required extensions (PhysicalDevice::kRequiredExtensions), the required features (dynamicRendering,
/// synchronization2) and every optional feature the physical device supports. It must be destroyed before the
/// instance.
class RTYPE_RENDER_VULKAN_API Device {
  public:
    /// @brief Creates the logical device and retrieves its graphics and present queues.
    /// @param physicalDevice The chosen physical device, with its queue families and optional features.
    /// @throws std::runtime_error If no physical device was selected.
    /// @throws vk::SystemError If the device cannot be created.
    explicit Device(const PhysicalDevice& physicalDevice);
    ~Device() = default;
    Device(const Device&) = delete;
    Device& operator=(const Device&) = delete;
    Device(Device&&) = delete;
    Device& operator=(Device&&) = delete;

    /// @return The logical device, used to create resources and submit commands to the GPU.
    [[nodiscard]] const vk::raii::Device& getDevice() const noexcept { return _device; }
    /// @return The queue that receives graphics commands.
    [[nodiscard]] const vk::raii::Queue& getGraphicsQueue() const noexcept { return _graphicsQueue; }
    /// @return The queue that presents swapchain images to the surface.
    [[nodiscard]] const vk::raii::Queue& getPresentQueue() const noexcept { return _presentQueue; }

  private:
    vk::raii::Device _device = nullptr;        ///< The logical device, destroyed on destruction.
    vk::raii::Queue _graphicsQueue = nullptr;  ///< Graphics queue, owned by _device.
    vk::raii::Queue _presentQueue = nullptr;   ///< Present queue, owned by _device; may be the graphics queue.
};
}  // namespace rtype::render::vulkan::core
