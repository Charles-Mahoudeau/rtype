/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FrameData
*/

#include "FrameData.hpp"

#include <utility>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::frame {

FrameData::FrameData(const core::Device& device)
    : _commandPool{device.getDevice(), vk::CommandPoolCreateInfo{}
                                           .setFlags(vk::CommandPoolCreateFlagBits::eTransient)
                                           .setQueueFamilyIndex(device.getGraphicsQueue().family)},
      _imageAvailable{device.getDevice(), vk::SemaphoreCreateInfo{}},
      _inFlight{device.getDevice(), vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled)} {
    vk::raii::CommandBuffers buffers{device.getDevice(), vk::CommandBufferAllocateInfo{}
                                                             .setCommandPool(*_commandPool)
                                                             .setLevel(vk::CommandBufferLevel::ePrimary)
                                                             .setCommandBufferCount(1)};
    _commandBuffer = std::move(buffers.front());
}

}  // namespace rtype::render::vulkan::frame
