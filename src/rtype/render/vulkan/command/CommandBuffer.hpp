/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandBuffer
*/

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::command {

/// @brief A command buffer, allocated by a CommandPool: what the GPU will execute, recorded between begin() and end().
///
/// @details Recording itself (draws, copies, barriers) goes through getCommandBuffer(), which every recording function
/// of the renderer takes.
/// @warning Destroy it before its CommandPool, and only once the GPU is done executing it.
class RTYPE_RENDER_VULKAN_API CommandBuffer {
  public:
    /// @brief Takes a command buffer allocated by CommandPool::allocate().
    explicit CommandBuffer(vk::raii::CommandBuffer commandBuffer) noexcept;
    ~CommandBuffer() = default;
    CommandBuffer(const CommandBuffer&) = delete;
    CommandBuffer& operator=(const CommandBuffer&) = delete;
    CommandBuffer(CommandBuffer&&) noexcept = default;
    CommandBuffer& operator=(CommandBuffer&&) noexcept = default;

    /// @return The Vulkan command buffer, to record into and to submit.
    [[nodiscard]] const vk::raii::CommandBuffer& getCommandBuffer() const noexcept { return _commandBuffer; }

    /// @brief Starts recording. Default: recorded for a single submission, then reset with its pool.
    /// @throws vk::SystemError If it fails.
    void begin(vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit) const;

    /// @brief Ends recording: it can then be submitted.
    /// @throws vk::SystemError If recording failed.
    void end() const;

  private:
    vk::raii::CommandBuffer _commandBuffer;  ///< Freed back to its pool on destruction.
};

}  // namespace rtype::render::vulkan::command
