/*
** EPITECH PROJECT, 2026
** $
** File description:
** Swapchain
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "../core/Device.hpp"
#include "../core/PhysicalDevice.hpp"

namespace rtype::render::vulkan::presentation {

class Swapchain {
  public:
    Swapchain(glm::uvec2 framebufferSize, const core::PhysicalDevice& physicalDevice,
              const vk::raii::SurfaceKHR& surface, const core::Device& device);
    ~Swapchain() = default;
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&&) = delete;
    Swapchain& operator=(Swapchain&&) = delete;

  private:
    vk::raii::SwapchainKHR _swapChain;  ///< The swapchain, destroyed on destruction. The images are owned by the
                                        ///< swapchain and destroyed with it.
};
}  // namespace rtype::render::vulkan::presentation
