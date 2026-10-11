/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FrameData
*/

#pragma once

#include "render/vulkan/Export.hpp"
#include "render/vulkan/command/CommandBuffer.hpp"
#include "render/vulkan/command/CommandPool.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/sync/Fence.hpp"
#include "render/vulkan/sync/Semaphore.hpp"

namespace rtype::render::vulkan::frame {

/// @brief What one frame in flight records into and synchronizes with.
///
/// @details While the GPU renders frame N with one FrameData, the CPU records frame N + 1 into another. Before reusing
/// a FrameData, wait for its inFlight fence: its command buffer and semaphore are then free again.
class RTYPE_RENDER_VULKAN_API FrameData {
  public:
    /// @brief Creates the command pool and buffer on the graphics queue family, and the sync objects.
    /// @throws vk::SystemError If one of them cannot be created.
    explicit FrameData(const core::Device& device);
    ~FrameData() = default;
    FrameData(const FrameData&) = delete;
    FrameData& operator=(const FrameData&) = delete;
    FrameData(FrameData&&) noexcept = default;
    FrameData& operator=(FrameData&&) noexcept = default;

    /// @return The command pool of the frame, transient and reset as a whole each frame.
    [[nodiscard]] const command::CommandPool& getCommandPool() const noexcept { return _commandPool; }
    /// @return The primary command buffer the frame is recorded into, allocated from getCommandPool().
    [[nodiscard]] const command::CommandBuffer& getCommandBuffer() const noexcept { return _commandBuffer; }
    /// @return Signaled when the acquired swapchain image is ready to be rendered into.
    [[nodiscard]] const sync::Semaphore& getImageAvailable() const noexcept { return _imageAvailable; }
    /// @return Signaled when the GPU has finished the frame's submission; created signaled, so the first wait
    /// returns at once.
    [[nodiscard]] const sync::Fence& getInFlight() const noexcept { return _inFlight; }

  private:
    command::CommandPool _commandPool;      ///< Owns _commandBuffer, destroyed after it.
    command::CommandBuffer _commandBuffer;  ///< Primary command buffer, freed before _commandPool.
    sync::Semaphore _imageAvailable;        ///< Acquire to rendering.
    sync::Fence _inFlight;                  ///< Submission to CPU.
};
}  // namespace rtype::render::vulkan::frame
