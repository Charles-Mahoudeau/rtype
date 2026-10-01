/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GlfwPlatformCallbacks
*/

#include <GLFW/glfw3.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>

#include "GlfwMapping.hpp"
#include "GlfwPlatform.hpp"
#include "engine/event/Event.hpp"

namespace rtype::platform {

/// @brief Installs the GLFW callbacks that turn window input into engine events.
///
/// @details GLFW only accepts plain function pointers, so each callback is a
/// static member that finds its GlfwPlatform again through the GLFW user pointer.
/// This is why GlfwPlatform can be neither copied nor moved: the stored `this` would dangle.
void GlfwPlatform::registerCallbacks() {
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

GlfwPlatform& GlfwPlatform::fromHandle(GLFWwindow* handle) {
    return *static_cast<GlfwPlatform*>(glfwGetWindowUserPointer(handle));
}

void GlfwPlatform::onClose(GLFWwindow* handle) { fromHandle(handle)._events.emplace_back(engine::event::Closed{}); }

void GlfwPlatform::onFramebufferResize(GLFWwindow* handle, int width, int height) {
    fromHandle(handle)._events.emplace_back(engine::event::Resized{.width = static_cast<std::uint16_t>(width),
                                                                   .height = static_cast<std::uint16_t>(height)});
}

void GlfwPlatform::onFocus(GLFWwindow* handle, int focused) {
    fromHandle(handle)._events.emplace_back(engine::event::FocusChanged{.focused = focused == GLFW_TRUE});
}

void GlfwPlatform::onKey(GLFWwindow* handle, int key, int /*scancode*/, int action, int mods) {
    GlfwPlatform& self = fromHandle(handle);
    const auto engineKey = glfw::fromGlfwKey(key);
    const auto engineMods = glfw::fromGlfwMods(mods);
    if (action == GLFW_RELEASE) {
        self._events.emplace_back(engine::event::KeyReleased{.key = engineKey, .mods = engineMods});
    } else {
        self._events.emplace_back(
            engine::event::KeyPressed{.key = engineKey, .mods = engineMods, .repeat = action == GLFW_REPEAT});
    }
}

void GlfwPlatform::onChar(GLFWwindow* handle, unsigned int codepoint) {
    fromHandle(handle)._events.emplace_back(engine::event::TextEntered{.codepoint = codepoint});
}

void GlfwPlatform::onCursorPos(GLFWwindow* handle, double x, double y) {
    GlfwPlatform& self = fromHandle(handle);
    const glm::vec2 position{static_cast<float>(x), static_cast<float>(y)};
    const glm::vec2 delta = self._hasCursorPosition ? position - self._cursorPosition : glm::vec2{0.0F};
    self._cursorPosition = position;
    self._hasCursorPosition = true;
    self._events.emplace_back(engine::event::MouseMoved{.position = position, .delta = delta});
}

void GlfwPlatform::onMouseButton(GLFWwindow* handle, int button, int action, int mods) {
    GlfwPlatform& self = fromHandle(handle);
    const auto engineButton = glfw::fromGlfwMouseButton(button);
    const auto engineMods = glfw::fromGlfwMods(mods);
    if (action == GLFW_RELEASE) {
        self._events.emplace_back(engine::event::MouseButtonReleased{.button = engineButton, .mods = engineMods});
    } else {
        self._events.emplace_back(engine::event::MouseButtonPressed{.button = engineButton, .mods = engineMods});
    }
}

void GlfwPlatform::onScroll(GLFWwindow* handle, double xoffset, double yoffset) {
    fromHandle(handle)._events.emplace_back(
        engine::event::MouseScrolled{.offset = {static_cast<float>(xoffset), static_cast<float>(yoffset)}});
}

}  // namespace rtype::platform
