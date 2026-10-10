/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FrameResources
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "FrameData.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::frame {

/// @brief The FrameData of every frame in flight, and one renderFinished semaphore per swapchain image.
///
/// @details renderFinished is per swapchain image, not per frame: presentation keeps waiting on it after the frame's
/// fence has signaled, so a per-frame semaphore could be signaled again while still pending. The image index returned
/// by the acquire picks it.
class RTYPE_RENDER_VULKAN_API FrameResources {
  public:
    /// @param device The logical device the objects are created on.
    /// @param framesInFlight Number of FrameData, i.e. frames the CPU may record ahead of the GPU. Must not be 0.
    /// @param swapchainImageCount Number of renderFinished semaphores, one per swapchain image.
    /// @throws std::invalid_argument If @p framesInFlight is 0.
    /// @throws vk::SystemError If an object cannot be created.
    FrameResources(const core::Device& device, std::size_t framesInFlight, std::size_t swapchainImageCount);
    ~FrameResources() = default;
    FrameResources(const FrameResources&) = delete;
    FrameResources& operator=(const FrameResources&) = delete;
    FrameResources(FrameResources&&) = delete;
    FrameResources& operator=(FrameResources&&) = delete;

    /// @return The FrameData of the frame being recorded.
    [[nodiscard]] const FrameData& getCurrentFrame() const { return _frames.at(_frameIndex); }
    /// @return The index of the frame being recorded, in [0, getFrameCount()).
    [[nodiscard]] std::size_t getFrameIndex() const noexcept { return _frameIndex; }
    /// @return The number of frames in flight.
    [[nodiscard]] std::size_t getFrameCount() const noexcept { return _frames.size(); }

    /// @return The semaphore signaled when rendering into swapchain image @p imageIndex is done.
    /// @throws std::out_of_range If @p imageIndex is not a swapchain image index.
    [[nodiscard]] const vk::raii::Semaphore& getRenderFinished(std::uint32_t imageIndex) const;

    /// @brief Moves on to the next frame in flight, wrapping around.
    void advance() noexcept;

  private:
    std::vector<FrameData> _frames;                    ///< One per frame in flight.
    std::vector<vk::raii::Semaphore> _renderFinished;  ///< One per swapchain image.
    std::size_t _frameIndex = 0;                       ///< Frame being recorded.
};
}  // namespace rtype::render::vulkan::frame
