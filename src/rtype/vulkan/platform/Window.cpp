/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Window
*/

#include "Window.hpp"

#include <GLFW/glfw3.h>

#include <cstdint>
#include <string>
#include <utility>

#include "exceptions/WindowExceptions.hpp"

namespace rtype::vulkan::platform {

/**
 * @brief Creates the window wrapper and initializes GLFW for Vulkan rendering.
 *
 * @details Initializes the GLFW library and sets the window hints required
 * before the native window is created. The window itself is not created here.
 *
 * Window hints:
 * - `GLFW_CLIENT_API = GLFW_NO_API`: by default GLFW creates an OpenGL context
 *   with the window. Vulkan does not use an OpenGL context, so it is disabled.
 *   The rendering surface is created later with `glfwCreateWindowSurface()`.
 * - `GLFW_RESIZABLE = GLFW_TRUE`: the user can resize the window. The renderer
 *   must then handle `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` and
 *   recreate the swapchain.
 *
 * @param width  Initial window width in pixels.
 * @param height Initial window height in pixels.
 * @param title  Text displayed in the window title bar.
 *
 * @throws GLFWWindowException If `glfwInit()` fails.
 *
 * @note A constructor has no return value, so there is no `@return`.
 * @see createSurface(), recreateSwapchain()
 */
Window::Window(std::uint16_t width, std::uint16_t height, std::string title)
    : _width(width), _height(height), _title(std::move(title)) {
    if (glfwInit() == GLFW_FALSE) {
        throw exceptions::GLFWWindowException("Failed to initialize GLFW");
    }
    _glfwInitialized = true;
    _window = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
    if (_window == nullptr) {
        glfwTerminate();
        throw exceptions::GLFWWindowException("Failed to create GLFW window");
    }
}

Window::~Window() {
    if (_window != nullptr) {
        glfwDestroyWindow(_window);
        if (_glfwInitialized) {
            glfwTerminate();
            _glfwInitialized = false;
        }
    }
}

GLFWwindow *Window::getHandle() const noexcept { return _window; }
}  // namespace rtype::vulkan::platform
