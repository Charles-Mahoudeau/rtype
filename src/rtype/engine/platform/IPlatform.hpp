/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** IPlatform
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <string>
#include <vector>

#include "engine/event/Event.hpp"
#include "engine/exceptions/PlatformExceptions.hpp"
#include "engine/platform/WindowConfig.hpp"

namespace rtype::engine::platform {

/// @brief Generic function pointer, to pass a loader entry point (e.g. vkGetInstanceProcAddr) without its type.
using ProcAddress = void (*)();

/// @brief The window and the input devices, implemented by a windowing backend (GLFW for now).
///
/// @details Every native event is translated into an engine Event: the engine, Input included, never sees the
/// windowing library. The engine starts a platform and a renderer together, in this order:
/// @code
/// platform.initLoader(renderer.getLoaderEntryPoint());  // optional on both sides
/// platform.init(config);                                 // creates the window
/// renderer.init(platform);                               // attaches the renderer to it
/// @endcode
/// The functions marked optional have a default implementation: a platform only overrides the ones a renderer it
/// is paired with needs.
class IPlatform {
  public:
    IPlatform() = default;
    virtual ~IPlatform() = default;
    IPlatform(const IPlatform&) = delete;
    IPlatform& operator=(const IPlatform&) = delete;
    IPlatform(IPlatform&&) = delete;
    IPlatform& operator=(IPlatform&&) = delete;

    /// @return Size of the framebuffer, in pixels. May differ from the window size on high-DPI screens.
    [[nodiscard]] virtual glm::uvec2 getFramebufferSize() const = 0;

    /// @return Current window title.
    [[nodiscard]] virtual const std::string& getTitle() const = 0;

    /// @brief Sets the window title.
    virtual void setTitle(const std::string& title) = 0;

    /// @brief Changes the window size, in screen coordinates.
    virtual void setSize(glm::uvec2 size) = 0;

    /// @brief Creates the window. Called once by the engine, after initLoader().
    virtual void init(const WindowConfig& config) = 0;

    /// @return Every event received since the previous call. Call once per frame.
    [[nodiscard]] virtual std::vector<Event> pollEvents() = 0;

    /// @return True while the window is open.
    [[nodiscard]] virtual bool isOpen() const noexcept = 0;

    /// @brief Closes the window.
    virtual void close() = 0;

    /// @brief Hides and locks the cursor. Mouse deltas keep flowing.
    virtual void setCursorLocked(bool locked) = 0;

    /// @return Seconds elapsed since init(), to compute the frame delta time.
    [[nodiscard]] virtual double getTime() const = 0;

    /// @name Optional: renderer integration
    /// @brief Only overridden by platforms paired with a renderer that needs them.
    /// @{

    /// @brief Makes the platform use the renderer's API loader instead of loading one of its own, so the process
    /// has a single one (GLFW: glfwInitVulkanLoader). Called before init(). Default: does nothing.
    /// @param entryPoint From IRenderer::getLoaderEntryPoint(); null when the renderer has none.
    virtual void initLoader(ProcAddress /*entryPoint*/) {}

    /// @return API extensions the window needs (Vulkan: VK_KHR_surface plus the OS one). Default: none.
    [[nodiscard]] virtual std::vector<const char*> getRequiredExtensions() const { return {}; }

    /// @brief Creates the surface of the window for a renderer that draws through one (Vulkan).
    /// @param instance The API instance (Vulkan: a VkInstance).
    /// @return The surface (Vulkan: a VkSurfaceKHR), owned by the renderer from then on.
    /// @throws exceptions::UnsupportedFeatureException By default: this platform cannot create a surface.
    [[nodiscard]] virtual std::uint64_t createSurface(void* /*instance*/) {
        throw exceptions::UnsupportedFeatureException("This platform cannot create a rendering surface");
    }
    /// @}
};
}  // namespace rtype::engine::platform
