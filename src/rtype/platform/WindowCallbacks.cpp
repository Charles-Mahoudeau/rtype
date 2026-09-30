/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowCallbacks
*/

#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>

#include "GlfwMapping.hpp"
#include "Window.hpp"
#include "engine/event/Event.hpp"

namespace rtype::platform {

/**
 * @brief Installs the GLFW callbacks that turn window input into engine events.
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
    glfwSetCursorPosCallback(_window, onCursorPos);
    glfwSetMouseButtonCallback(_window, onMouseButton);
    glfwSetScrollCallback(_window, onScroll);
}

Window& Window::fromHandle(GLFWwindow* handle) { return *static_cast<Window*>(glfwGetWindowUserPointer(handle)); }

void Window::onClose(GLFWwindow* handle) { fromHandle(handle)._events.emplace_back(engine::event::Closed{}); }

void Window::onFramebufferResize(GLFWwindow* handle, int width, int height) {
    Window& self = fromHandle(handle);
    self._width = static_cast<std::uint16_t>(width);
    self._height = static_cast<std::uint16_t>(height);
    self._events.emplace_back(engine::event::Resized{.width = self._width, .height = self._height});
}

void Window::onFocus(GLFWwindow* handle, int focused) {
    fromHandle(handle)._events.emplace_back(engine::event::FocusChanged{.focused = focused == GLFW_TRUE});
}

void Window::onKey(GLFWwindow* handle, int key, int /*scancode*/, int action, int mods) {
    Window& self = fromHandle(handle);
    const auto engineKey = glfw::fromGlfwKey(key);
    const auto engineMods = glfw::fromGlfwMods(mods);
    if (action == GLFW_RELEASE) {
        self._events.emplace_back(engine::event::KeyReleased{.key = engineKey, .mods = engineMods});
    } else {
        self._events.emplace_back(
            engine::event::KeyPressed{.key = engineKey, .mods = engineMods, .repeat = action == GLFW_REPEAT});
    }
}

void Window::onChar(GLFWwindow* handle, unsigned int codepoint) {
    fromHandle(handle)._events.emplace_back(engine::event::TextEntered{.codepoint = codepoint});
}

void Window::onCursorPos(GLFWwindow* handle, double x, double y) {
    Window& self = fromHandle(handle);
    const glm::vec2 position{static_cast<float>(x), static_cast<float>(y)};
    const glm::vec2 delta = self._hasCursorPosition ? position - self._cursorPosition : glm::vec2{0.0F};
    self._cursorPosition = position;
    self._hasCursorPosition = true;
    self._events.emplace_back(engine::event::MouseMoved{.position = position, .delta = delta});
}

void Window::onMouseButton(GLFWwindow* handle, int button, int action, int mods) {
    Window& self = fromHandle(handle);
    const auto engineButton = glfw::fromGlfwMouseButton(button);
    const auto engineMods = glfw::fromGlfwMods(mods);
    if (action == GLFW_RELEASE) {
        self._events.emplace_back(engine::event::MouseButtonReleased{.button = engineButton, .mods = engineMods});
    } else {
        self._events.emplace_back(engine::event::MouseButtonPressed{.button = engineButton, .mods = engineMods});
    }
}

void Window::onScroll(GLFWwindow* handle, double xoffset, double yoffset) {
    fromHandle(handle)._events.emplace_back(
        engine::event::MouseScrolled{.offset = {static_cast<float>(xoffset), static_cast<float>(yoffset)}});
}

}  // namespace rtype::platform
