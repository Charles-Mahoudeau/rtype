/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CommandLabel
*/

#include "CommandLabel.hpp"

#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::command {

CommandLabel::CommandLabel(const core::Device& device, const vk::raii::CommandBuffer& commandBuffer, const char* name) {
    if (device.hasDebugUtils()) {
        commandBuffer.beginDebugUtilsLabelEXT(vk::DebugUtilsLabelEXT{}.setPLabelName(name));
        _commandBuffer = &commandBuffer;
    }
}

CommandLabel::~CommandLabel() {
    if (_commandBuffer != nullptr) {
        _commandBuffer->endDebugUtilsLabelEXT();
    }
}

}  // namespace rtype::render::vulkan::command
