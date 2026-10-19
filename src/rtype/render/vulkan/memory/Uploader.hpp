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
#include "Image.hpp"
#include "render/vulkan/Export.hpp"
#include "render/vulkan/command/CommandBuffer.hpp"
#include "render/vulkan/command/CommandPool.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/sync/Fence.hpp"

namespace rtype::render::vulkan::memory {

/// @brief Fills buffers the CPU cannot write (device local memory) and images through staging buffers, several at
/// once.
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
/// An image is always staged (its tiling is optimal), and left in SHADER_READ_ONLY_OPTIMAL, ready to be sampled.
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

    /// @brief Copies @p texels into every texel of @p destination at the next flush(), which leaves it in
    /// SHADER_READ_ONLY_OPTIMAL. Its previous contents are discarded.
    /// @param texels Row by row from the top, tightly packed, in the image's format.
    /// @param destination Needs eTransferDst in its usage, and eSampled to be read by shaders afterwards.
    /// @throws std::invalid_argument If @p texels is not exactly the image's size in bytes.
    /// @throws vk::SystemError If the staging buffer or the command buffer cannot be created.
    void upload(std::span<const std::byte> texels, Image& destination);

    /// @brief Submits the recorded uploads and waits for them, then frees their staging buffers. Does nothing when
    /// none is pending.
    /// @throws vk::SystemError If the submission fails.
    /// @throws std::runtime_error If waiting for it fails.
    void flush();

  private:
    /// @brief Begins the command buffer if this is the first upload since the last flush().
    void beginRecording();

    const core::Device* _device;            ///< Records and submits; not owned.
    const Allocator* _allocator;            ///< Allocates the staging buffers; not owned.
    command::CommandPool _commandPool;      ///< Transient pool of the command buffer, reset by flush().
    command::CommandBuffer _commandBuffer;  ///< Records the copies; freed before _commandPool.
    sync::Fence _fence;                     ///< Signaled when a flush's copies are done.
    std::vector<Buffer> _stagingBuffers;    ///< Sources of the recorded copies, freed by flush().
    bool _recording = false;                ///< Whether the command buffer has been begun.
};

}  // namespace rtype::render::vulkan::memory
