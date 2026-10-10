/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Allocator
*/

#pragma once

#include <cstdint>
#include <string>
#include <vk_mem_alloc_raii.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/core/Instance.hpp"
#include "render/vulkan/core/PhysicalDevice.hpp"

namespace rtype::render::vulkan::memory {

/// @brief The VMA allocator of the device: every buffer and image of the renderer gets its memory from it.
/// @details VMA sub-allocates large blocks and picks the memory type, instead of one vkAllocateMemory per resource.
/// Buffers and images are returned as vma::raii objects that own both the resource and its memory: destroying them
/// frees both. They must all be destroyed before the Allocator, which must be destroyed before the Device. In debug
/// builds, the destructor prints the VMA statistics, which list the allocations still alive (leaks).
class RTYPE_RENDER_VULKAN_API Allocator {
  public:
    /// @brief How the CPU accesses a resource; picks the VMA flags (memory usage is always VMA_MEMORY_USAGE_AUTO).
    enum class MemoryAccess : std::uint8_t {
        kGpuOnly,   ///< Never touched by the CPU: DEVICE_LOCAL. Textures, render targets, static vertex data.
        kStaging,   ///< Written once by the CPU, then copied: always HOST_VISIBLE | HOST_COHERENT, and mapped.
        kUpload,    ///< Written by the CPU, read by the GPU: mapped DEVICE_LOCAL memory when it is host visible
                    ///< (Apple Silicon, ReBAR), so no staging is needed; otherwise not host visible, check
                    ///< isHostVisible() and copy through a kStaging buffer.
        kReadback,  ///< Written by the GPU, read by the CPU: HOST_VISIBLE, mapped, cached when possible.
    };

    /// @brief Creates the allocator for @p device, targeting Vulkan 1.3.
    /// @details Enables buffer device addresses when the physical device supports them (Device enables the feature).
    /// @throws vk::SystemError If VMA cannot create the allocator.
    Allocator(const core::Instance& instance, const core::PhysicalDevice& physicalDevice, const core::Device& device);
    /// @brief Prints the VMA statistics in debug builds, then destroys the allocator.
    ~Allocator();
    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;
    Allocator(Allocator&&) = delete;
    Allocator& operator=(Allocator&&) = delete;

    /// @return The VMA allocator, for what the helpers below do not cover.
    [[nodiscard]] const vma::raii::Allocator& getAllocator() const noexcept { return _allocator; }

    /// @brief Creates a buffer and its memory.
    /// @param bufferInfo The buffer to create (size, usage...).
    /// @param access How the CPU accesses it; host-visible ones are persistently mapped (getAllocation().getInfo()).
    /// @throws vk::SystemError If the buffer or its memory cannot be created.
    [[nodiscard]] vma::raii::Buffer createBuffer(const vk::BufferCreateInfo& bufferInfo, MemoryAccess access) const;

    /// @brief Creates an image and its memory.
    /// @param imageInfo The image to create (format, extent, usage...).
    /// @param access How the CPU accesses it; kGpuOnly for nearly every image.
    /// @throws vk::SystemError If the image or its memory cannot be created.
    [[nodiscard]] vma::raii::Image createImage(const vk::ImageCreateInfo& imageInfo,
                                               MemoryAccess access = MemoryAccess::kGpuOnly) const;

    /// @return Whether @p allocation is host visible, i.e. can be written through its mapping. A kUpload resource
    /// that is not must be filled through a kStaging buffer.
    [[nodiscard]] static bool isHostVisible(const vma::raii::Allocation& allocation);

    /// @return The VMA statistics as JSON: memory heaps, types and blocks, and with @p detailed every allocation.
    [[nodiscard]] std::string buildStatsString(bool detailed) const;

  private:
    vma::raii::Allocator _allocator = nullptr;  ///< The VMA allocator, destroyed on destruction.
};
}  // namespace rtype::render::vulkan::memory
