/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Buffer
*/

#pragma once

#include <cstddef>
#include <span>
#include <vk_mem_alloc_raii.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"
#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::memory {

/// @brief A buffer and its memory, allocated by the Allocator: vertices, indices, uniforms, staging...
///
/// @details A convenience layer over vma::raii::Buffer that keeps the size and usage, and writes into the memory when
/// the CPU can reach it (MemoryAccess kStaging, kReadback, and kUpload on unified memory, which stay mapped). Device
/// local memory is filled through an Uploader instead.
/// @warning Destroy it before the Allocator, and only once the GPU is done with it (see core::DeletionQueue).
class RTYPE_RENDER_VULKAN_API Buffer {
  public:
    /// @param allocator Allocates the buffer and its memory; must outlive the buffer.
    /// @param size Its size, in bytes; not 0.
    /// @param usage How it is used: eVertexBuffer, eIndexBuffer, eUniformBuffer... plus eTransferDst to receive an
    /// upload, eTransferSrc to be the source of a copy.
    /// @param access How the CPU accesses its memory.
    /// @throws std::invalid_argument If @p size is 0.
    /// @throws vk::SystemError If the buffer or its memory cannot be created.
    Buffer(const Allocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage, Allocator::MemoryAccess access);
    ~Buffer() = default;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    Buffer(Buffer&&) noexcept = default;
    Buffer& operator=(Buffer&&) noexcept = default;

    /// @return The Vulkan buffer and its allocation; *getBuffer() is the vk::Buffer to bind or copy.
    [[nodiscard]] const vma::raii::Buffer& getBuffer() const noexcept { return _buffer; }
    /// @return Its size, in bytes.
    [[nodiscard]] vk::DeviceSize getSize() const noexcept { return _size; }
    /// @return Its usage flags.
    [[nodiscard]] vk::BufferUsageFlags getUsage() const noexcept { return _usage; }
    /// @return Whether the CPU can write it with write(); otherwise use an Uploader.
    [[nodiscard]] bool isHostVisible() const noexcept { return _mapped != nullptr; }

    /// @brief Copies @p data into the buffer at @p offset, then flushes it so the GPU sees it.
    /// @throws std::logic_error If the buffer is not CPU-accessible (isHostVisible() is false).
    /// @throws std::out_of_range If @p offset + data.size() is greater than getSize().
    void write(std::span<const std::byte> data, vk::DeviceSize offset = 0);

  private:
    vma::raii::Buffer _buffer;    ///< The buffer and its memory, freed on destruction.
    vk::DeviceSize _size;         ///< In bytes.
    vk::BufferUsageFlags _usage;  ///< How it is used.
    void* _mapped = nullptr;      ///< Where the CPU writes it; null when it cannot.
};

}  // namespace rtype::render::vulkan::memory
