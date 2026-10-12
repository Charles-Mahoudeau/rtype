/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Fence
*/

#include "Fence.hpp"

#include <cstdint>
#include <stdexcept>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/core/Device.hpp"

namespace rtype::render::vulkan::sync {

Fence::Fence(const core::Device& device, bool signaled)
    : _device{&device},
      _fence{device.getDevice(),
             vk::FenceCreateInfo{}.setFlags(signaled ? vk::FenceCreateFlagBits::eSignaled : vk::FenceCreateFlags{})} {}

bool Fence::isSignaled() const { return _fence.getStatus() == vk::Result::eSuccess; }

void Fence::wait(std::uint64_t timeout) const {
    if (_device->getDevice().waitForFences(*_fence, vk::True, timeout) != vk::Result::eSuccess) {
        throw std::runtime_error("Fence: the wait timed out");
    }
}

void Fence::reset() const { _device->getDevice().resetFences(*_fence); }

}  // namespace rtype::render::vulkan::sync
