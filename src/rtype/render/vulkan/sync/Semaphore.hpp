/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Semaphore
*/

#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::sync {

/// @brief GPU to GPU synchronization between submissions: signaled by one (or by the swapchain's acquire), waited on
/// by another (or by presentation).
///
/// @details A binary semaphore, passed to vkQueueSubmit2 and vkQueuePresentKHR through getSemaphore(). Timeline
/// semaphores (a counter the CPU can also wait on and signal) will join it when they are needed.
/// @warning Destroy it before the Device, and only once no pending operation waits on or signals it.
class RTYPE_RENDER_VULKAN_API Semaphore {
  public:
    /// @param device The device the semaphore is created on.
    /// @throws vk::SystemError If the semaphore cannot be created.
    explicit Semaphore(const core::Device& device);
    ~Semaphore() = default;
    Semaphore(const Semaphore&) = delete;
    Semaphore& operator=(const Semaphore&) = delete;
    Semaphore(Semaphore&&) noexcept = default;
    Semaphore& operator=(Semaphore&&) noexcept = default;

    /// @return The Vulkan semaphore.
    [[nodiscard]] const vk::raii::Semaphore& getSemaphore() const noexcept { return _semaphore; }

  private:
    vk::raii::Semaphore _semaphore;  ///< Destroyed on destruction.
};

}  // namespace rtype::render::vulkan::sync
