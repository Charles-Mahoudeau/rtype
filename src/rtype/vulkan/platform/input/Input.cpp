/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Input
*/

#include "Input.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <cmath>
#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <string>

#include "platform/exceptions/InputExceptions.hpp"
#include "platform/input/Control.hpp"
#include "platform/input/InputAction.hpp"

namespace rtype::vulkan::platform::input {

Input::Input(GLFWwindow* window) : _window(window) {}

void Input::update() {
    updateMouse();
    updateGamepad();

    for (auto& [name, action] : _actions) {
        glm::vec2 best{0.0F};
        for (const auto& binding : action._bindings) {
            const glm::vec2 value = evaluate(binding);
            if (glm::length(value) > glm::length(best)) {
                best = value;
            }
        }
        action.update(best);
    }
}

void Input::updateMouse() {
    glm::dvec2 position{0.0};
    glfwGetCursorPos(_window, &position.x, &position.y);
    if (_firstMouse) {
        _mousePosition = position;
        _firstMouse = false;
    }
    _mouseDelta = position - _mousePosition;
    _mousePosition = position;
}

void Input::updateGamepad() {
    _gamepadConnected = false;
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
        if (glfwJoystickIsGamepad(jid) == GLFW_TRUE && glfwGetGamepadState(jid, &_gamepad) == GLFW_TRUE) {
            _gamepadConnected = true;
            return;
        }
    }
}

glm::vec2 Input::evaluate(const InputAction::Binding& binding) const {
    glm::vec2 value{0.0F};
    for (const auto& part : binding.parts) {
        value += read(part.control) * part.direction;
    }
    if (binding.normalize && glm::length(value) > 1.0F) {
        value = glm::normalize(value);
    }
    return value;
}

float Input::read(const Control& control) const {
    switch (control.source) {
        case Control::Source::Key:
            if (control.code < GLFW_KEY_SPACE || control.code > GLFW_KEY_LAST) {
                return 0.0F;
            }
            return glfwGetKey(_window, control.code) == GLFW_PRESS ? control.scale : 0.0F;
        case Control::Source::MouseButton:
            if (control.code < 0 || control.code > GLFW_MOUSE_BUTTON_LAST) {
                return 0.0F;
            }
            return glfwGetMouseButton(_window, control.code) == GLFW_PRESS ? control.scale : 0.0F;
        case Control::Source::MouseDeltaX:
            return static_cast<float>(_mouseDelta.x) * control.scale;
        case Control::Source::MouseDeltaY:
            return static_cast<float>(-_mouseDelta.y) * control.scale;
        case Control::Source::GamepadButton:
            if (!_gamepadConnected || control.code < 0 || control.code > GLFW_GAMEPAD_BUTTON_LAST) {
                return 0.0F;
            }
            return std::to_array(_gamepad.buttons).at(control.code) == GLFW_PRESS ? control.scale : 0.0F;
        case Control::Source::GamepadAxis:
            return readGamepadAxis(control.code) * control.scale;
    }
    return 0.0F;
}

float Input::readGamepadAxis(int axis) const {
    if (!_gamepadConnected || axis < 0 || axis > GLFW_GAMEPAD_AXIS_LAST) {
        return 0.0F;
    }
    float value = std::to_array(_gamepad.axes).at(axis);
    if (axis == GLFW_GAMEPAD_AXIS_LEFT_TRIGGER || axis == GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER) {
        value = (value + 1.0F) * 0.5F;
    } else if (axis == GLFW_GAMEPAD_AXIS_LEFT_Y || axis == GLFW_GAMEPAD_AXIS_RIGHT_Y) {
        value = -value;
    }
    const float magnitude = std::abs(value);
    if (magnitude < _deadzone) {
        return 0.0F;
    }
    return std::copysign((magnitude - _deadzone) / (1.0F - _deadzone), value);
}

InputAction& Input::addAction(const std::string& name, ActionType type) {
    return _actions.insert_or_assign(name, InputAction(type)).first->second;
}

void Input::removeAction(const std::string& name) {
    auto it = _actions.find(name);
    if (it == _actions.end()) {
        throw exceptions::InputException("Unknown input action: " + name);
    }
    _actions.erase(it);
}

void Input::clearActions() noexcept { _actions.clear(); }

InputAction& Input::getAction(const std::string& name) {
    auto it = _actions.find(name);
    if (it == _actions.end()) {
        throw exceptions::InputException("Unknown input action: " + name);
    }
    return it->second;
}

const InputAction& Input::getAction(const std::string& name) const {
    auto it = _actions.find(name);
    if (it == _actions.end()) {
        throw exceptions::InputException("Unknown input action: " + name);
    }
    return it->second;
}

glm::vec2 Input::getMousePosition() const noexcept { return _mousePosition; }

bool Input::isGamepadConnected() const noexcept { return _gamepadConnected; }

void Input::setDeadzone(float deadzone) noexcept { _deadzone = deadzone; }

void Input::setCursorLocked(bool locked) {
    glfwSetInputMode(_window, GLFW_CURSOR, locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    _firstMouse = true;
}

}  // namespace rtype::vulkan::platform::input
