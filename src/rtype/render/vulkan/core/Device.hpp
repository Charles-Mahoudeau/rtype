/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Device
*/

#pragma once

#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

#include "PhysicalDevice.hpp"
#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::core {

/// @brief The logical device created from the chosen PhysicalDevice, and its graphics, present, compute and transfer
/// queues.
/// @details The logical device is the handle used to create resources and submit commands to the GPU. It enables the
/// extensions the physical device lists (PhysicalDevice::getDeviceExtensions()), the required features
/// (dynamicRendering, synchronization2) and every optional feature the physical device supports. It must be destroyed
/// before the instance.
class RTYPE_RENDER_VULKAN_API Device {
  public:
    /// @brief A queue of the device, with the family it comes from (needed for ownership transfers and pools).
    /// @details Roles that share a family share the same queue: submitting to it from several threads needs a lock.
    struct Queue {
        vk::raii::Queue handle = nullptr;  ///< The queue, owned by the device.
        std::uint32_t family = 0;          ///< Index of the queue family it belongs to.
    };

    /// @brief Creates the logical device and retrieves its queues.
    /// @param physicalDevice The chosen physical device, with its queue families, extensions and optional features.
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
    [[nodiscard]] const Queue& getGraphicsQueue() const noexcept { return _graphicsQueue; }
    /// @return The queue that presents swapchain images to the surface.
    [[nodiscard]] const Queue& getPresentQueue() const noexcept { return _presentQueue; }
    /// @return The queue that receives compute commands.
    [[nodiscard]] const Queue& getComputeQueue() const noexcept { return _computeQueue; }
    /// @return The queue for buffer and image copies.
    [[nodiscard]] const Queue& getTransferQueue() const noexcept { return _transferQueue; }

  private:
    vk::raii::Device _device = nullptr;  ///< The logical device, destroyed on destruction.
    Queue _graphicsQueue;                ///< Graphics queue.
    Queue _presentQueue;                 ///< Present queue; may be the graphics queue.
    Queue _computeQueue;                 ///< Compute queue; may be the graphics queue.
    Queue _transferQueue;                ///< Transfer queue; may be the graphics queue.
};
}  // namespace rtype::render::vulkan::core
