/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Window
*/

#pragma once

#include <cstdint>
#include <string>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace rtype::vulkan::platform {
/**
 * @brief A wrapper for the GLFW window.
 *
 * This class provides a simple interface for creating and managing a GLFW window
 * that is intended for use with Vulkan rendering.
 */
class Window {
  public:
    Window(std::uint16_t width, std::uint16_t height, std::string title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    [[nodiscard]] GLFWwindow* getHandle() const noexcept;

    /// @brief Implicit conversion to the underlying GLFW handle, e.g. Input input(window).
    operator GLFWwindow*() const noexcept { return _window; }

    /// @return True while the window is open.
    [[nodiscard]] bool isOpen() const noexcept;

    /// @return Current framebuffer width in pixels.
    [[nodiscard]] std::uint16_t getWidth() const noexcept;

    /// @return Current framebuffer height in pixels.
    [[nodiscard]] std::uint16_t getHeight() const noexcept;

    /// @return Current framebuffer size in pixels.
    void getSize(std::uint16_t& width, std::uint16_t& height) const noexcept;

    /// @return Current window title.
    [[nodiscard]] const std::string& getTitle() const noexcept;

    /// @brief Sets the window title.
    void setTitle(const std::string& title);

    /// @brief Change the window size in pixels.
    void setSize(std::uint16_t width, std::uint16_t height);

    /// @brief Polls for window events and updates the window state.
    void pollEvents();

  protected:
  private:
    bool _isOpen{true};            ///< True while the window is open.
    bool _glfwInitialized{false};  ///< True once glfwInit() succeeded, used to call glfwTerminate().
    GLFWwindow* _window{nullptr};  ///< The GLFW window instance
    std::uint16_t _width;          ///< Current framebuffer width in pixels.
    std::uint16_t _height;         ///< Current framebuffer height in pixels.
    std::string _title;            ///< Window title.
};
}  // namespace rtype::vulkan::platform
