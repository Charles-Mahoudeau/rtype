/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Window
*/

#ifndef WINDOW_HPP_
#define WINDOW_HPP_

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
#endif /* !WINDOW_HPP_ */
