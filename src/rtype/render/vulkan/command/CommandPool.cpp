/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandPool
*/

#include "CommandPool.hpp"

#include <cstdint>
#include <utility>
#include <vulkan/vulkan_raii.hpp>

#include "CommandBuffer.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::command {

CommandPool::CommandPool(const core::Device& device, std::uint32_t queueFamily, vk::CommandPoolCreateFlags flags)
    : _device{&device},
      _pool{device.getDevice(), vk::CommandPoolCreateInfo{}.setFlags(flags).setQueueFamilyIndex(queueFamily)} {}

CommandBuffer CommandPool::allocate(vk::CommandBufferLevel level) const {
    vk::raii::CommandBuffers buffers{
        _device->getDevice(),
        vk::CommandBufferAllocateInfo{}.setCommandPool(*_pool).setLevel(level).setCommandBufferCount(1)};
    return CommandBuffer{std::move(buffers.front())};
}

void CommandPool::reset() const { _pool.reset(); }

}  // namespace rtype::render::vulkan::command
