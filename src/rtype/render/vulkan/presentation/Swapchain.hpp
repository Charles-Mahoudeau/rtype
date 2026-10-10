/*
** EPITECH PROJECT, 2026
** $
** File description:
** Swapchain
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <vulkan/vulkan_raii.hpp>

#include "../core/Device.hpp"
#include "../core/PhysicalDevice.hpp"

namespace rtype::render::vulkan::presentation {

class Swapchain {
  public:
    Swapchain(glm::uvec2 framebufferSize, const core::PhysicalDevice& physicalDevice,
              const vk::raii::SurfaceKHR& surface, const core::Device& device);
    ~Swapchain();
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&&) = delete;

    /// @brief Overload of the move assignment operator, used to move the swapchain into a new one when recreating it.
    Swapchain& operator=(Swapchain&& other) noexcept {
        if (this != &other) {
            _swapChain = std::move(other._swapChain);
            _imageViews = std::move(other._imageViews);
            _framebuffers = std::move(other._framebuffers);
            renderPass = std::move(other.renderPass);
        }
        return *this;
    }

    /// @brief Recreates the swapchain, image views and framebuffers.
    /// @param framebufferSize The new size of the swapchain, in pixels.
    /// @param physicalDevice The physical device, used to query the swapchain support.
    /// @param surface The surface, used to create the swapchain.
    /// @param device The device, used to create the swapchain, image views and framebuffers.
    void recreate(glm::uvec2 framebufferSize, const core::PhysicalDevice& physicalDevice,
                  const vk::raii::SurfaceKHR& surface, const core::Device& device);

    /// @return The swapchain, used to acquire images and present them to the surface.
    [[nodiscard]] const vk::raii::SwapchainKHR& getSwapChain() const noexcept { return _swapChain; }
    /// @return The swapchain image views, used to render into the images.
    [[nodiscard]] const std::vector<vk::raii::ImageView>& getImageViews() const noexcept { return _imageViews; }

    /// @return The swapchain image count, used to create one framebuffer per image.
    [[nodiscard]] uint32_t getImageCount() const noexcept { return _swapChain.getImages().size(); }

    /// @return The swapchain images, used to render into them.
    [[nodiscard]] std::vector<vk::Image> getImages() const noexcept { return _swapChain.getImages(); }
    /// @return The framebuffers, used to render into the swapchain images.
    [[nodiscard]] const std::vector<vk::raii::Framebuffer>& getFramebuffers() const noexcept { return _framebuffers; }

    /// @return The render pass, used to render into the swapchain images.
    [[nodiscard]] const vk::raii::RenderPass& getRenderPass() const noexcept { return renderPass; }

  private:
    /// @brief Creates the framebuffers, one per swapchain image view.
    /// @param device The device, used to create the framebuffers.
    /// @param swapChainExtent The size of the framebuffers, in pixels.
    void createFramebuffers(const core::Device& device, const vk::Extent2D& swapChainExtent);
    /// @brief Creates the image views, one per swapchain image.
    /// @param device The device, used to create the image views.
    /// @param swapChainImageFormat The format of the swapchain images.
    void createImageViews(const core::Device& device, vk::Format swapChainImageFormat);
    /// @brief Creates the render pass, used to render into the swapchain images.
    /// @param device The device, used to create the render pass.
    /// @param swapChainImageFormat The format of the swapchain images.
    static vk::raii::RenderPass createRenderPass(const core::Device& device, vk::Format swapChainImageFormat);

    vk::raii::SwapchainKHR _swapChain;  ///< The swapchain, destroyed on destruction. The images are owned by the
                                        ///< swapchain and destroyed with it.
    std::vector<vk::raii::ImageView>
        _imageViews;  ///< The swapchain images, owned by the swapchain and destroyed with it.

    std::vector<vk::raii::Framebuffer> _framebuffers;  ///< The framebuffers, destroyed on destruction. The images are
                                                       ///< owned by the swapchain and destroyed with it.

    vk::raii::RenderPass renderPass;
};
}  // namespace rtype::render::vulkan::presentation
