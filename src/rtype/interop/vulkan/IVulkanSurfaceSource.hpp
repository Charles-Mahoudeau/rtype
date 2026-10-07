/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** IVulkanSurfaceSource
*/

#pragma once

#include <vulkan/vulkan_core.h>

#include <vector>

#include "interop/Export.hpp"

namespace rtype::interop::vulkan {

/// @brief What a Vulkan renderer needs from a window, implemented by the platforms that can serve one.
///
/// @details A platform implements it next to IPlatform (GlfwPlatform, the SDL example's SdlPlatform). A Vulkan
/// renderer finds it with a dynamic_cast, and throws if the platform does not implement it: the pair does not fit.
/// It only depends on the Vulkan headers, so the platform and the renderer never depend on each other.
class RTYPE_INTEROP_API IVulkanSurfaceSource {
  public:
    IVulkanSurfaceSource() = default;
    virtual ~IVulkanSurfaceSource() = default;
    IVulkanSurfaceSource(const IVulkanSurfaceSource&) = delete;
    IVulkanSurfaceSource& operator=(const IVulkanSurfaceSource&) = delete;
    IVulkanSurfaceSource(IVulkanSurfaceSource&&) = delete;
    IVulkanSurfaceSource& operator=(IVulkanSurfaceSource&&) = delete;

    /// @brief Makes the windowing library use the renderer's Vulkan loader instead of loading one of its own, so
    /// the process has a single one (GLFW: glfwInitVulkanLoader()).
    /// @note Called before IPlatform::init(), from IRenderer::setup(): the window does not exist yet.
    virtual void initLoader(PFN_vkGetInstanceProcAddr loader) = 0;

    /// @return The instance extensions the window's surface needs: VK_KHR_surface plus the OS one
    /// (VK_EXT_metal_surface, VK_KHR_win32_surface...). The strings are owned by the platform.
    [[nodiscard]] virtual std::vector<const char*> getRequiredExtensions() const = 0;

    /// @brief Creates the surface of the window.
    /// @return The surface, owned by the caller from then on.
    [[nodiscard]] virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;
};
}  // namespace rtype::interop::vulkan
