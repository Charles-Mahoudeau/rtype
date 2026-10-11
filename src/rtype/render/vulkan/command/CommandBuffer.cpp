/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandBuffer
*/

#include "CommandBuffer.hpp"

#include <utility>
#include <vulkan/vulkan_raii.hpp>

namespace rtype::render::vulkan::command {

CommandBuffer::CommandBuffer(vk::raii::CommandBuffer commandBuffer) noexcept
    : _commandBuffer{std::move(commandBuffer)} {}

void CommandBuffer::begin(vk::CommandBufferUsageFlags flags) const {
    _commandBuffer.begin(vk::CommandBufferBeginInfo{}.setFlags(flags));
}

void CommandBuffer::end() const { _commandBuffer.end(); }

}  // namespace rtype::render::vulkan::command
