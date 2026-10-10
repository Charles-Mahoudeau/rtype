/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FrameResources
*/

#include "FrameResources.hpp"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

#include "FrameData.hpp"
#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::frame {

FrameResources::FrameResources(const core::Device& device, std::size_t framesInFlight,
                               std::size_t swapchainImageCount) {
    if (framesInFlight == 0) {
        throw std::invalid_argument("FrameResources needs at least one frame in flight");
    }
    _frames.reserve(framesInFlight);
    for (std::size_t frame = 0; frame < framesInFlight; ++frame) {
        _frames.emplace_back(device);
    }
    _renderFinished.reserve(swapchainImageCount);
    for (std::size_t image = 0; image < swapchainImageCount; ++image) {
        _renderFinished.emplace_back(device.getDevice(), vk::SemaphoreCreateInfo{});
    }
}

const vk::raii::Semaphore& FrameResources::getRenderFinished(std::uint32_t imageIndex) const {
    return _renderFinished.at(imageIndex);
}

void FrameResources::advance() noexcept { _frameIndex = (_frameIndex + 1) % _frames.size(); }

}  // namespace rtype::render::vulkan::frame
