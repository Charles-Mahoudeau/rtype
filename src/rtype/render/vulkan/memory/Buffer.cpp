/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Buffer
*/

#include "Buffer.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <span>
#include <stdexcept>
#include <vk_mem_alloc_raii.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"

namespace rtype::render::vulkan::memory {

namespace {

/// @return A buffer of @p size bytes for @p usage, after checking the size.
/// @throws std::invalid_argument If @p size is 0, which Vulkan does not allow.
vma::raii::Buffer createBuffer(const Allocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage,
                               Allocator::MemoryAccess access) {
    if (size == 0) {
        throw std::invalid_argument("Buffer size must be greater than 0");
    }
    return allocator.createBuffer(
        vk::BufferCreateInfo{}.setSize(size).setUsage(usage).setSharingMode(vk::SharingMode::eExclusive), access);
}

}  // namespace

Buffer::Buffer(const Allocator& allocator, vk::DeviceSize size, vk::BufferUsageFlags usage,
               Allocator::MemoryAccess access)
    : _buffer{createBuffer(allocator, size, usage, access)},
      _size{size},
      _usage{usage},
      _mapped{_buffer.getAllocation().getInfo().pMappedData} {}

void Buffer::write(std::span<const std::byte> data, vk::DeviceSize offset) {
    if (_mapped == nullptr) {
        throw std::logic_error("Buffer is not CPU-accessible: upload into it with an Uploader");
    }
    if (offset > _size || data.size() > _size - offset) {
        throw std::out_of_range(std::format("Buffer write out of range: {} + {} > {}", offset, data.size(), _size));
    }
    const std::span<std::byte> mapped{static_cast<std::byte*>(_mapped), static_cast<std::size_t>(_size)};
    std::ranges::copy(data, mapped.subspan(static_cast<std::size_t>(offset)).begin());
    _buffer.getAllocation().flush(offset, data.size());
}

}  // namespace rtype::render::vulkan::memory
