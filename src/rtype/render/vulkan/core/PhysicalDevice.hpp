/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicalDevice
*/

#pragma once

#include <array>
#include <cstdint>
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
    /// @brief Queue families of the chosen device. All are guaranteed to exist: a device without them is never chosen.
    /// @details Several roles may share the same family.
    struct QueueFamilies {
        std::uint32_t graphics = 0;  ///< Family that supports graphics commands.
        std::uint32_t present = 0;   ///< Family that can present to the surface.
        std::uint32_t compute = 0;   ///< Family that supports compute, a dedicated one (async compute) if any.
        std::uint32_t transfer = 0;  ///< Family for copies, a dedicated one (DMA) if any, the graphics one otherwise.
    };

    /// @brief Features the renderer uses when the chosen device supports them, without requiring them.
    struct OptionalFeatures {
        bool timelineSemaphore = false;    ///< Semaphores with a 64-bit counter, to sync CPU and GPU without fences.
        bool samplerAnisotropy = false;    ///< Anisotropic filtering of textures.
        bool bufferDeviceAddress = false;  ///< GPU addresses of buffers, usable in shaders.
        bool descriptorIndexing = false;   ///< Bindless descriptors (non-uniform indexing, partially bound arrays...).
    };

    /// @brief Limits of the chosen device the renderer needs.
    struct Limits {
        vk::DeviceSize minUniformBufferOffsetAlignment = 0;  ///< Alignment of dynamic uniform buffer offsets.
        std::uint32_t maxPushConstantsSize = 0;              ///< Maximum size of push constants, in bytes.
        vk::SampleCountFlags framebufferColorSampleCounts;   ///< MSAA sample counts supported by color attachments.
        float timestampPeriod = 0.0F;                        ///< Nanoseconds per timestamp query tick.
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

    /// @brief Device extensions a GPU must support to be chosen.
    static constexpr std::array<const char*, 1> kRequiredExtensions{vk::KHRSwapchainExtensionName};
    /// @brief Exposed by non-conformant implementations (MoltenVK); the spec requires enabling it when exposed.
    static constexpr const char* kPortabilitySubsetExtension = "VK_KHR_portability_subset";

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

    /// @return The optional features the chosen device supports.
    [[nodiscard]] const OptionalFeatures& getOptionalFeatures() const noexcept { return _optionalFeatures; }

    /// @return The limits of the chosen device.
    [[nodiscard]] const Limits& getLimits() const noexcept { return _limits; }

    /// @return The graphics, present, compute and transfer queue families of the chosen device.
    [[nodiscard]] const QueueFamilies& getQueueFamilies() const noexcept { return _queueFamilies; }

    /// @return The number of queue families of the chosen device.
    [[nodiscard]] const std::uint32_t getQueueFamiliesIndexCount() const noexcept { return _queueFamiliesIndexCount; }

    /// @return The extensions the logical device must enable: kRequiredExtensions, plus kPortabilitySubsetExtension
    /// when the chosen device exposes it.
    [[nodiscard]] const std::vector<const char*>& getDeviceExtensions() const noexcept { return _deviceExtensions; }

    /// @brief Picks the first suitable physical device from the Vulkan instance.
    /// @param surface The Vulkan surface to check for swap chain support.
    /// @param preferredType The type of GPU favored when several suitable ones are found.
    /// @throws std::runtime_error If no suitable physical device is found.
    void pickPhysicalDevice(const vk::raii::SurfaceKHR& surface, vk::PhysicalDeviceType preferredType);

    /// @brief Queries the swap chain support of a physical device.
    /// @param device The physical device to query.
    /// @param surface The surface to check for swap chain support.
    /// @return The swap chain support details.
    static PhysicalDevice::SwapChainSupportDetails querySwapChainSupport(const vk::raii::PhysicalDevice& device,
                                                                         const vk::raii::SurfaceKHR& surface);

  private:
    vk::raii::PhysicalDevices _physicalDevices;  ///< All physical devices of the instance, destroyed on destruction.
    vk::raii::PhysicalDevice _physicalDevice;  ///< The first physical device of the instance, destroyed on destruction.
    OptionalFeatures _optionalFeatures;        ///< Optional features the chosen device supports.
    Limits _limits;                            ///< Limits of the chosen device.
    QueueFamilies _queueFamilies;              ///< Queue families of the chosen device.
    uint32_t _queueFamiliesIndexCount = 0;     ///< Index of the graphics queue family.
    std::vector<const char*> _deviceExtensions;  ///< Extensions the logical device must enable.
};
}  // namespace rtype::render::vulkan::core
