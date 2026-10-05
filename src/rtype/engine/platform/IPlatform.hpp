/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** IPlatform
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <string>
#include <vector>

#include "engine/Export.hpp"
#include "engine/event/Event.hpp"
#include "engine/platform/WindowConfig.hpp"

namespace rtype::engine::platform {

/// @brief The window and the input devices, implemented by a windowing backend (GLFW for now).
///
/// @details Every native event is translated into an engine Event: the engine, Input included, never sees the
/// windowing library. The engine starts a platform and a renderer together (BackendRegistry::createBackend()):
/// @code
/// renderer.prepare(platform);  // the renderer sets the platform up, before the window exists
/// platform.init(config);       // creates the window
/// renderer.init(platform);     // attaches the renderer to it
/// @endcode
/// What a renderer needs beyond this interface (a Vulkan surface...) comes from an interop interface the platform
/// also implements (rtype::interop::vulkan::IVulkanSurfaceSource): IPlatform stays free of any graphics API.
class RTYPE_ENGINE_API IPlatform {
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
};
}  // namespace rtype::engine::platform
