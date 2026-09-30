/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Window
*/

#include "Window.hpp"

#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/ext/vector_double2.hpp>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "engine/event/Event.hpp"
#include "exceptions/WindowExceptions.hpp"

namespace rtype::platform {

/// @brief Initializes GLFW and creates a window ready for Vulkan rendering.
///
/// @details Window hints:
/// - `GLFW_CLIENT_API = GLFW_NO_API`: by default GLFW creates an OpenGL context
///   with the window. Vulkan does not use an OpenGL context, so it is disabled.
///   The rendering surface is created later with `glfwCreateWindowSurface()`.
/// - `GLFW_RESIZABLE = GLFW_TRUE`: the user can resize the window. The renderer
///   must then handle `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` and
///   recreate the swapchain.
///
/// @param width  Initial window width in pixels.
/// @param height Initial window height in pixels.
/// @param title  Text displayed in the window title bar.
///
/// @throws GLFWWindowException If `glfwInit()` or the window creation fails.
Window::Window(std::uint16_t width, std::uint16_t height, std::string title)
    : _width(width), _height(height), _title(std::move(title)) {
    if (glfwInit() == GLFW_FALSE) {
        throw exceptions::GLFWWindowException("Failed to initialize GLFW");
    }
    _glfwInitialized = true;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    _window = glfwCreateWindow(_width, _height, _title.c_str(), nullptr, nullptr);
    if (_window == nullptr) {
        // The exception is built before glfwTerminate(), which would discard the GLFW error it reports.
        try {
            throw exceptions::GLFWWindowException("Failed to create GLFW window");
        } catch (...) {
            glfwTerminate();
            throw;
        }
    }
    registerCallbacks();
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

std::vector<engine::Event> Window::pollEvents() {
    _events.clear();
    glfwPollEvents();
    pollGamepad();
    _isOpen = glfwWindowShouldClose(_window) == GLFW_FALSE;
    return std::exchange(_events, {});
}

bool Window::isOpen() const noexcept { return _isOpen; }

void Window::close() {
    _isOpen = false;
    glfwSetWindowShouldClose(_window, GLFW_TRUE);
}

void Window::getSize(std::uint16_t& width, std::uint16_t& height) const noexcept {
    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(_window, &framebufferWidth, &framebufferHeight);
    width = static_cast<std::uint16_t>(framebufferWidth);
    height = static_cast<std::uint16_t>(framebufferHeight);
}

std::uint16_t Window::getWidth() const noexcept {
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    getSize(width, height);
    return width;
}

std::uint16_t Window::getHeight() const noexcept {
    std::uint16_t width = 0;
    std::uint16_t height = 0;
    getSize(width, height);
    return height;
}

const std::string& Window::getTitle() const noexcept { return _title; }

void Window::setTitle(const std::string& title) {
    _title = title;
    glfwSetWindowTitle(_window, _title.c_str());
}

void Window::setSize(std::uint16_t width, std::uint16_t height) {
    _width = width;
    _height = height;
    glfwSetWindowSize(_window, _width, _height);
}

void Window::setCursorLocked(bool locked) {
    glfwSetInputMode(_window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported() == GLFW_TRUE) {
        glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, locked ? GLFW_TRUE : GLFW_FALSE);
    }
    glm::dvec2 position{0.0};
    glfwGetCursorPos(_window, &position.x, &position.y);
    _cursorPosition = position;
}

std::vector<const char*> Window::getRequiredVulkanExtensions() {
    uint32_t glfwExtensionCount = 0;
    auto* extensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (extensions == nullptr) {
        throw exceptions::GLFWWindowException("Vulkan is not supported: no required instance extensions");
    }
    const std::span<const char*> view(extensions, glfwExtensionCount);
    return {view.begin(), view.end()};
}

GLFWwindow* Window::getNativeHandle() const noexcept { return _window; }

}  // namespace rtype::platform
