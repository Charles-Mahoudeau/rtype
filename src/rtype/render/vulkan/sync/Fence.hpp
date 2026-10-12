/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Fence
*/

#pragma once

#include <cstdint>
#include <limits>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::sync {

/// @brief GPU to CPU synchronization: signaled by the GPU when a submission is done, waited on by the CPU.
///
/// @details Passed to vkQueueSubmit2 through getFence(). wait() and reset() wrap vkWaitForFences and vkResetFences,
/// with their result checked in one place.
/// @warning Destroy it before the Device, and only once no pending submission signals it.
class RTYPE_RENDER_VULKAN_API Fence {
  public:
    /// @brief wait() without a timeout.
    static constexpr std::uint64_t kNoTimeout = std::numeric_limits<std::uint64_t>::max();

    /// @param device The device the fence is created on; must outlive it.
    /// @param signaled Whether it starts signaled, so the first wait() returns at once (e.g. a frame's fence).
    /// @throws vk::SystemError If the fence cannot be created.
    explicit Fence(const core::Device& device, bool signaled = false);
    ~Fence() = default;
    Fence(const Fence&) = delete;
    Fence& operator=(const Fence&) = delete;
    Fence(Fence&&) noexcept = default;
    Fence& operator=(Fence&&) noexcept = default;

    /// @return The Vulkan fence, for vkQueueSubmit2.
    [[nodiscard]] const vk::raii::Fence& getFence() const noexcept { return _fence; }

    /// @return Whether it is signaled, without waiting.
    /// @throws vk::SystemError If the device is lost.
    [[nodiscard]] bool isSignaled() const;

    /// @brief Blocks until it is signaled, at most @p timeout nanoseconds.
    /// @throws std::runtime_error If the timeout expires first.
    /// @throws vk::SystemError If the device is lost.
    void wait(std::uint64_t timeout = kNoTimeout) const;

    /// @brief Makes it unsignaled again, before passing it to a new submission.
    /// @throws vk::SystemError If it fails.
    void reset() const;

  private:
    const core::Device* _device;  ///< Waits on and resets the fence; not owned.
    vk::raii::Fence _fence;       ///< Destroyed on destruction.
};

}  // namespace rtype::render::vulkan::sync
