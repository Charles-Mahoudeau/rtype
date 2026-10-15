/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandPool
*/

#pragma once

#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

#include "CommandBuffer.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::command {

/// @brief Allocates the command buffers of one queue family, and resets them all at once.
///
/// @details Not thread-safe: one pool per thread that records. The default (transient) suits buffers recorded for a
/// single submission and reset as a whole with reset(), like a frame's or an upload's.
/// @warning Destroy every CommandBuffer it allocated before it, and it before the Device.
class RTYPE_RENDER_VULKAN_API CommandPool {
  public:
    /// @param device The device the pool and its buffers are created on; must outlive the pool.
    /// @param queueFamily The family of the queue its buffers are submitted to, e.g. Device::getGraphicsQueue().family.
    /// @param flags eTransient by default; eResetCommandBuffer to reset its buffers one by one.
    /// @throws vk::SystemError If the pool cannot be created.
    CommandPool(const core::Device& device, std::uint32_t queueFamily,
                vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eTransient);
    ~CommandPool() = default;
    CommandPool(const CommandPool&) = delete;
    CommandPool& operator=(const CommandPool&) = delete;
    CommandPool(CommandPool&&) noexcept = default;
    CommandPool& operator=(CommandPool&&) noexcept = default;

    /// @return The Vulkan pool.
    [[nodiscard]] const vk::raii::CommandPool& getPool() const noexcept { return _pool; }

    /// @return A new command buffer of @p level.
    /// @throws vk::SystemError If it cannot be allocated.
    [[nodiscard]] CommandBuffer allocate(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary) const;

    /// @brief Resets every command buffer it allocated, to record them again. None may be executing.
    /// @throws vk::SystemError If it fails.
    void reset() const;

  private:
    const core::Device* _device;  ///< Allocates the buffers; not owned.
    vk::raii::CommandPool _pool;  ///< Destroyed on destruction.
};

}  // namespace rtype::render::vulkan::command
