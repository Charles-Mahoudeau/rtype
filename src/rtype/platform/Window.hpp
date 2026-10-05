/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Window
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <vector>

#include "engine/event/Event.hpp"
#include "engine/input/Key.hpp"

struct GLFWwindow;

namespace rtype::platform {

/// @brief The application window, and the source of every engine event.
///
/// @details Wraps a GLFW window created for Vulkan rendering. GLFW stays an
/// implementation detail: this header does not include it, and every callback is
/// translated into an engine::Event returned by pollEvents().
///
/// @warning On macOS 11.3+, controllers natively handled by Apple's GameController
/// framework (e.g. Switch Pro Controller) are detected by GLFW but never send updates,
/// so they read as idle.
class Window {
  public:
    /// @throws GLFWWindowException If GLFW or the window cannot be initialized.
    Window(std::uint16_t width, std::uint16_t height, std::string title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    /// @return Every event received since the previous call. Call once per frame.
    std::vector<engine::Event> pollEvents();

    /// @return True while the window is open.
    [[nodiscard]] bool isOpen() const noexcept;

    /// @brief Closes the window.
    void close();

    /// @return Current framebuffer width in pixels.
    /// @note Prefer getSize() to read both at once, the window may be resized between two calls.
    [[nodiscard]] std::uint16_t getWidth() const noexcept;

    /// @return Current framebuffer height in pixels.
    /// @note Prefer getSize() to read both at once, the window may be resized between two calls.
    [[nodiscard]] std::uint16_t getHeight() const noexcept;

    /// @brief Reads the current framebuffer size in pixels.
    void getSize(std::uint16_t& width, std::uint16_t& height) const noexcept;

    /// @return Current window title.
    [[nodiscard]] const std::string& getTitle() const noexcept;

    /// @brief Sets the window title.
    void setTitle(const std::string& title);

    /// @brief Changes the window size in screen coordinates.
    void setSize(std::uint16_t width, std::uint16_t height);

    /// @brief Hides and locks the cursor, useful for an FPS camera. Mouse deltas keep flowing.
    void setCursorLocked(bool locked);

    /// @return The underlying GLFW window.
    /// @note Only for integrations that require it (Vulkan surface, ImGui backend).
    /// Game and engine code must go through the events instead.
    [[nodiscard]] GLFWwindow* getNativeHandle() const noexcept;

  private:
    static constexpr std::size_t kGamepadButtonCount = static_cast<std::size_t>(engine::input::GamepadButton::kCount);
    static constexpr std::size_t kGamepadAxisCount = static_cast<std::size_t>(engine::input::GamepadAxis::kCount);

    /// @brief Binds this instance to the GLFW handle and installs every event callback.
    void registerCallbacks();

    /// @return The Window bound to a GLFW handle by registerCallbacks().
    static Window& fromHandle(GLFWwindow* handle);

    /// @name GLFW callbacks
    /// @brief Translate a GLFW callback into an engine event pushed to _events.
    /// @{
    static void onClose(GLFWwindow* handle);
    static void onFramebufferResize(GLFWwindow* handle, int width, int height);
    static void onFocus(GLFWwindow* handle, int focused);
    static void onKey(GLFWwindow* handle, int key, int scancode, int action, int mods);
    static void onChar(GLFWwindow* handle, unsigned int codepoint);
    static void onCursorPos(GLFWwindow* handle, double x, double y);
    static void onMouseButton(GLFWwindow* handle, int button, int action, int mods);
    static void onScroll(GLFWwindow* handle, double xoffset, double yoffset);
    /// @}

    /// @brief GLFW has no gamepad callbacks: compares the gamepad with the previous frame and emits the changes.
    void pollGamepad();
    /// @brief Releases every button and axis, then emits GamepadDisconnected.
    void disconnectGamepad();

    bool _isOpen{true};                  ///< True while the window is open.
    bool _glfwInitialized{false};        ///< True once glfwInit() succeeded, used to call glfwTerminate().
    GLFWwindow* _window{nullptr};        ///< The GLFW window instance.
    std::uint16_t _width;                ///< Framebuffer width in pixels, updated on resize.
    std::uint16_t _height;               ///< Framebuffer height in pixels, updated on resize.
    std::string _title;                  ///< Window title.
    std::vector<engine::Event> _events;  ///< Events received since the last poll.
    glm::vec2 _cursorPosition{0.0F};     ///< Last cursor position, to compute MouseMoved::delta.
    bool _hasCursorPosition{false};      ///< False until the first cursor event, to avoid a first jump.
    int _gamepadId{-1};                  ///< GLFW joystick id of the reported gamepad, -1 if none.
    std::array<bool, kGamepadButtonCount> _gamepadButtons{};  ///< Button state sent in the last events.
    std::array<float, kGamepadAxisCount> _gamepadAxes{};      ///< Axis values sent in the last events.
};
}  // namespace rtype::platform
