/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Swapchain
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"
#include "render/vulkan/core/Device.hpp"
#include "render/vulkan/core/PhysicalDevice.hpp"

namespace rtype::render::vulkan::presentation {

/// @brief The swapchain of the window's surface, and one image view per swapchain image.
///
/// @details Format B8G8R8A8_SRGB + SRGB_NONLINEAR when available, the first one otherwise. The present mode is the
/// requested one when the surface supports it, FIFO otherwise (vsync, always available). The images are color
/// attachments (rendered into with dynamic rendering), and transfer destinations when the surface allows it.
///
/// @note Recreating it (resize, VK_ERROR_OUT_OF_DATE_KHR) is not handled yet: it will build a new Swapchain from the
/// old one (oldSwapchain) and defer the old one's destruction.
class RTYPE_RENDER_VULKAN_API Swapchain {
  public:
    /// @brief Creates the swapchain and its image views.
    /// @param physicalDevice The chosen GPU: surface support and queue families.
    /// @param surface The window's surface.
    /// @param device The logical device that owns the swapchain.
    /// @param framebufferSize Size of the window's framebuffer, in pixels; used when the surface lets the
    /// application choose the extent.
    /// @param presentMode The preferred present mode; FIFO when the surface does not support it.
    /// @throws std::runtime_error If the framebuffer is empty (minimized window) or the surface cannot be rendered to.
    /// @throws vk::SystemError If the swapchain or an image view cannot be created.
    Swapchain(const core::PhysicalDevice& physicalDevice, const vk::raii::SurfaceKHR& surface,
              const core::Device& device, glm::uvec2 framebufferSize, vk::PresentModeKHR presentMode);
    ~Swapchain() = default;
    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;
    Swapchain(Swapchain&&) = delete;
    Swapchain& operator=(Swapchain&&) = delete;

    /// @return The swapchain, to acquire and present its images.
    [[nodiscard]] const vk::raii::SwapchainKHR& getSwapchain() const noexcept { return _swapchain; }
    /// @return The swapchain images, owned by the swapchain.
    [[nodiscard]] const std::vector<vk::Image>& getImages() const noexcept { return _images; }
    /// @return One image view per swapchain image, in the same order: the color attachments to render into.
    [[nodiscard]] const std::vector<vk::raii::ImageView>& getImageViews() const noexcept { return _imageViews; }
    /// @return The format of the images, for pipelines and dynamic rendering.
    [[nodiscard]] vk::Format getFormat() const noexcept { return _format; }
    /// @return The color space of the images.
    [[nodiscard]] vk::ColorSpaceKHR getColorSpace() const noexcept { return _colorSpace; }
    /// @return The size of the images, in pixels: viewport, scissor and render area.
    [[nodiscard]] vk::Extent2D getExtent() const noexcept { return _extent; }
    /// @return The present mode actually used.
    [[nodiscard]] vk::PresentModeKHR getPresentMode() const noexcept { return _presentMode; }

  private:
    /// @brief Creates one image view per swapchain image, in _imageViews.
    void createImageViews(const core::Device& device);

    vk::raii::SwapchainKHR _swapchain = nullptr;   ///< The swapchain; owns _images, destroyed after _imageViews.
    std::vector<vk::Image> _images;                ///< The swapchain images, owned by _swapchain.
    std::vector<vk::raii::ImageView> _imageViews;  ///< One view per image, destroyed before _swapchain.
    vk::Format _format = vk::Format::eUndefined;   ///< Format of the images.
    vk::ColorSpaceKHR _colorSpace{};               ///< Color space of the images.
    vk::Extent2D _extent;                          ///< Size of the images, in pixels.
    vk::PresentModeKHR _presentMode{};             ///< Present mode in use.
};
}  // namespace rtype::render::vulkan::presentation
