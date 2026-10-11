/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Uploader
*/

#pragma once

#include <cstddef>
#include <span>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"
#include "Buffer.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::memory {

/// @brief Fills buffers the CPU cannot write (device local memory) through staging buffers, several at once.
///
/// @details upload() only records: the data goes into a new staging buffer, and a copy plus a barrier into the
/// command buffer. flush() then submits everything in one go and waits for it, so loading many buffers costs one round
/// trip to the GPU:
/// @code
/// uploader.upload(vertices, vertexBuffer);
/// uploader.upload(indices, indexBuffer);
/// uploader.flush();
/// @endcode
/// A destination the CPU can write (unified memory: Apple Silicon, ReBAR) is written at once, without staging. The
/// barrier makes the copies visible to vertex and index fetching, uniform and storage reads of every later submission.
/// @warning Destroy it before the Allocator and the Device. flush() before using the destinations.
class RTYPE_RENDER_VULKAN_API Uploader {
  public:
    /// @param device Records and submits the copies on its graphics queue; must outlive the uploader.
    /// @param allocator Allocates the staging buffers; must outlive the uploader.
    /// @throws vk::SystemError If the command pool or the fence cannot be created.
    Uploader(const core::Device& device, const Allocator& allocator);
    /// @brief Flushes what is still pending; an error is reported, not thrown.
    ~Uploader();
    Uploader(const Uploader&) = delete;
    Uploader& operator=(const Uploader&) = delete;
    Uploader(Uploader&&) = delete;
    Uploader& operator=(Uploader&&) = delete;

    /// @return Whether uploads are waiting for flush().
    [[nodiscard]] bool hasPendingUploads() const noexcept { return _recording; }

    /// @brief Copies @p data into @p destination at @p offset: at once if the CPU can write it, at the next flush()
    /// otherwise (the destination then needs eTransferDst in its usage).
    /// @throws std::out_of_range If @p offset + data.size() is greater than the destination's size.
    /// @throws vk::SystemError If the staging buffer or the command buffer cannot be created.
    void upload(std::span<const std::byte> data, Buffer& destination, vk::DeviceSize offset = 0);

    /// @brief Submits the recorded uploads and waits for them, then frees their staging buffers. Does nothing when
    /// none is pending.
    /// @throws vk::SystemError If the submission fails.
    /// @throws std::runtime_error If waiting for it fails.
    void flush();

  private:
    const core::Device* _device;                       ///< Records and submits; not owned.
    const Allocator* _allocator;                       ///< Allocates the staging buffers; not owned.
    vk::raii::CommandPool _commandPool;                ///< Transient pool of the command buffer, reset by flush().
    vk::raii::CommandBuffer _commandBuffer = nullptr;  ///< Allocated by the first upload, reused afterwards.
    vk::raii::Fence _fence;                            ///< Signaled when a flush's copies are done.
    std::vector<Buffer> _stagingBuffers;               ///< Sources of the recorded copies, freed by flush().
    bool _recording = false;                           ///< Whether the command buffer has been begun.
};

}  // namespace rtype::render::vulkan::memory
