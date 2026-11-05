/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FrameResources
*/

#include "FrameResources.hpp"

#include <cstddef>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <utility>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "FrameData.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/sync/Semaphore.hpp"

namespace rtype::render::vulkan::frame {

FrameResources::FrameResources(const core::Device& device, std::size_t framesInFlight,
                               std::size_t swapchainImageCount) {
    if (framesInFlight == 0) {
        throw std::invalid_argument("FrameResources needs at least one frame in flight");
    }
    _frames.reserve(framesInFlight);
    for (std::size_t frame = 0; frame < framesInFlight; ++frame) {
        const FrameData& data = _frames.emplace_back(device);
        device.setName(*data.getCommandPool().getPool(), std::format("Frame {} command pool", frame));
        device.setName(*data.getCommandBuffer().getCommandBuffer(), std::format("Frame {} command buffer", frame));
        device.setName(*data.getImageAvailable().getSemaphore(), std::format("Frame {} image available", frame));
        device.setName(*data.getInFlight().getFence(), std::format("Frame {} in flight", frame));
    }
    _renderFinished = createSemaphores(device, swapchainImageCount);
}

std::vector<sync::Semaphore> FrameResources::recreateRenderFinished(const core::Device& device,
                                                                    std::size_t swapchainImageCount) {
    return std::exchange(_renderFinished, createSemaphores(device, swapchainImageCount));
}

std::vector<sync::Semaphore> FrameResources::createSemaphores(const core::Device& device, std::size_t count) {
    std::vector<sync::Semaphore> semaphores;
    semaphores.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const sync::Semaphore& semaphore = semaphores.emplace_back(device);
        device.setName(*semaphore.getSemaphore(), std::format("Image {} render finished", index));
    }
    return semaphores;
}

const sync::Semaphore& FrameResources::getRenderFinished(std::uint32_t imageIndex) const {
    return _renderFinished.at(imageIndex);
}

void FrameResources::advance() noexcept { _frameIndex = (_frameIndex + 1) % _frames.size(); }

}  // namespace rtype::render::vulkan::frame
