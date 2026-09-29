/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowCallbacks
*/

#include <GLFW/glfw3.h>

#include <cstdint>

#include "Event.hpp"
#include "Window.hpp"

namespace rtype::vulkan::platform {

/**
 * @brief Installs the GLFW callbacks that turn window input into Events.
 *
 * @details GLFW only accepts plain function pointers, so each callback is a
 * static member that finds its Window again through the GLFW user pointer.
 * This is why Window can be neither copied nor moved: the stored `this` would dangle.
 */
void Window::registerCallbacks() {
    glfwSetWindowUserPointer(_window, this);
    glfwSetWindowCloseCallback(_window, onClose);
    glfwSetFramebufferSizeCallback(_window, onFramebufferResize);
    glfwSetWindowFocusCallback(_window, onFocus);
    glfwSetKeyCallback(_window, onKey);
    glfwSetCharCallback(_window, onChar);
    glfwSetMouseButtonCallback(_window, onMouseButton);
    glfwSetScrollCallback(_window, onScroll);
}

Window& Window::fromHandle(GLFWwindow* handle) { return *static_cast<Window*>(glfwGetWindowUserPointer(handle)); }

void Window::onClose(GLFWwindow* handle) { fromHandle(handle)._events.emplace_back(event::Closed{}); }

void Window::onFramebufferResize(GLFWwindow* handle, int width, int height) {
    Window& self = fromHandle(handle);
    self._width = static_cast<std::uint16_t>(width);
    self._height = static_cast<std::uint16_t>(height);
    self._events.emplace_back(event::Resized{.width = self._width, .height = self._height});
}

void Window::onFocus(GLFWwindow* handle, int focused) {
    fromHandle(handle)._events.emplace_back(event::FocusChanged{.focused = focused == GLFW_TRUE});
}

void Window::onKey(GLFWwindow* handle, int key, int /*scancode*/, int action, int mods) {
    Window& self = fromHandle(handle);
    if (action == GLFW_RELEASE) {
        self._events.emplace_back(event::KeyReleased{.key = key, .mods = mods});
    } else {
        self._events.emplace_back(event::KeyPressed{.key = key, .mods = mods, .repeat = action == GLFW_REPEAT});
    }
}

void Window::onChar(GLFWwindow* handle, unsigned int codepoint) {
    fromHandle(handle)._events.emplace_back(event::TextEntered{.codepoint = codepoint});
}

void Window::onMouseButton(GLFWwindow* handle, int button, int action, int mods) {
    Window& self = fromHandle(handle);
    if (action == GLFW_RELEASE) {
        self._events.emplace_back(event::MouseButtonReleased{.button = button, .mods = mods});
    } else {
        self._events.emplace_back(event::MouseButtonPressed{.button = button, .mods = mods});
    }
}

void Window::onScroll(GLFWwindow* handle, double xoffset, double yoffset) {
    fromHandle(handle)._events.emplace_back(
        event::MouseScrolled{.offset = {static_cast<float>(xoffset), static_cast<float>(yoffset)}});
}

}  // namespace rtype::vulkan::platform
