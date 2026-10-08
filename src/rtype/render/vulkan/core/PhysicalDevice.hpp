/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicalDevice
*/

#pragma once

#include <cstdint>
#include <optional>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "Instance.hpp"
#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::core {
class RTYPE_RENDER_VULKAN_API PhysicalDevice {
    /// @brief The first physical device of a Vulkan instance, and all of them for lifetime management.
    /// @details The first physical device is the one used by the renderer. The others are kept alive so that the first
    /// one is not destroyed when the instance is destroyed.
    /// @note The physical devices are destroyed in reverse order.
  public:
    /// @brief Indices of the queue families of a physical device.
    struct QueueFamilyIndices {
        std::optional<std::uint32_t> graphicsFamily;  ///< Index of a queue family that supports graphics commands.
        std::optional<std::uint32_t> presentFamily;   ///< Index of a queue family that can present to the surface.

        /// @return Whether both a graphics and a present queue family were found.
        [[nodiscard]] bool isComplete() const noexcept { return graphicsFamily && presentFamily; }
    };

    /// @brief Details of the swap chain support of a physical device.
    /// @details This structure holds information about the capabilities, formats, and presentation modes of a Vulkan
    /// surface.
    struct SwapChainSupportDetails {
        vk::SurfaceCapabilitiesKHR
            capabilities;  ///< Basic surface capabilities (min/max number of images, min/max width and height, etc.).
        std::vector<vk::SurfaceFormatKHR> formats;  /// Available surface formats (pixel format, color space).
        std::vector<vk::PresentModeKHR>
            presentModes;  ///< Available presentation modes (how images are presented to the screen).
    };

    /// @brief Creates a PhysicalDevice object and picks the first suitable physical device from the Vulkan instance.
    /// @param instance The Vulkan instance.
    /// @param surface The Vulkan surface to check for swap chain support.
    /// @param preferredType The type of GPU favored when several suitable ones are found.
    PhysicalDevice(const Instance& instance, const vk::raii::SurfaceKHR& surface,
                   vk::PhysicalDeviceType preferredType = vk::PhysicalDeviceType::eDiscreteGpu);
    ~PhysicalDevice() = default;
    PhysicalDevice(const PhysicalDevice&) = delete;
    PhysicalDevice& operator=(const PhysicalDevice&) = delete;
    PhysicalDevice(PhysicalDevice&&) = delete;
    PhysicalDevice& operator=(PhysicalDevice&&) = delete;

    /// @return The first suitable physical device of the Vulkan instance.
    [[nodiscard]] const vk::raii::PhysicalDevice& getPhysicalDevice() const noexcept { return _physicalDevice; }

    /// @return All physical devices of the Vulkan instance, for lifetime management.
    [[nodiscard]] const vk::raii::PhysicalDevices& getPhysicalDevices() const noexcept { return _physicalDevices; }

    /// @brief Picks the first suitable physical device from the Vulkan instance.
    /// @param surface The Vulkan surface to check for swap chain support.
    /// @param preferredType The type of GPU favored when several suitable ones are found.
    /// @throws std::runtime_error If no suitable physical device is found.
    void pickPhysicalDevice(const vk::raii::SurfaceKHR& surface, vk::PhysicalDeviceType preferredType);

  protected:
  private:
    vk::raii::PhysicalDevices _physicalDevices;  ///< All physical devices of the instance, destroyed on destruction.
    vk::raii::PhysicalDevice _physicalDevice;  ///< The first physical device of the instance, destroyed on destruction.
};
}  // namespace rtype::render::vulkan::core
