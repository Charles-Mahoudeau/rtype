/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowGamepad
*/

#include <GLFW/glfw3.h>

#include <array>
#include <cstddef>

#include "GlfwMapping.hpp"
#include "Window.hpp"
#include "engine/event/Event.hpp"
#include "engine/input/Key.hpp"

namespace rtype::platform {

using engine::input::GamepadAxis;
using engine::input::GamepadButton;

namespace {

/// @return The first joystick id that GLFW recognizes as a gamepad, or -1.
int findGamepad() noexcept {
    for (int jid = GLFW_JOYSTICK_1; jid <= GLFW_JOYSTICK_LAST; ++jid) {
        if (glfwJoystickIsGamepad(jid) == GLFW_TRUE) {
            return jid;
        }
    }
    return -1;
}

}  // namespace

/// @details Only the first connected gamepad is reported. Axis values are converted
/// to the engine convention here, so Input only has to apply its deadzone.
void Window::pollGamepad() {
    if (_gamepadId >= 0 && glfwJoystickIsGamepad(_gamepadId) == GLFW_FALSE) {
        disconnectGamepad();
    }
    if (_gamepadId < 0) {
        _gamepadId = findGamepad();
        if (_gamepadId < 0) {
            return;
        }
        _events.emplace_back(engine::event::GamepadConnected{});
    }

    GLFWgamepadstate state{};
    if (glfwGetGamepadState(_gamepadId, &state) == GLFW_FALSE) {
        disconnectGamepad();
        return;
    }
    const auto buttons = std::to_array(state.buttons);
    const auto axes = std::to_array(state.axes);

    for (std::size_t i = 0; i < kGamepadButtonCount; ++i) {
        const auto button = static_cast<GamepadButton>(i);
        const bool down = buttons.at(static_cast<std::size_t>(glfw::toGlfwGamepadButton(button))) == GLFW_PRESS;
        if (down == _gamepadButtons.at(i)) {
            continue;
        }
        _gamepadButtons.at(i) = down;
        if (down) {
            _events.emplace_back(engine::event::GamepadButtonPressed{.button = button});
        } else {
            _events.emplace_back(engine::event::GamepadButtonReleased{.button = button});
        }
    }

    for (std::size_t i = 0; i < kGamepadAxisCount; ++i) {
        const auto axis = static_cast<GamepadAxis>(i);
        const float raw = axes.at(static_cast<std::size_t>(glfw::toGlfwGamepadAxis(axis)));
        const float value = glfw::fromGlfwGamepadAxisValue(axis, raw);
        if (value != _gamepadAxes.at(i)) {
            _gamepadAxes.at(i) = value;
            _events.emplace_back(engine::event::GamepadAxisMoved{.axis = axis, .value = value});
        }
    }
}

void Window::disconnectGamepad() {
    for (std::size_t i = 0; i < kGamepadButtonCount; ++i) {
        if (_gamepadButtons.at(i)) {
            _events.emplace_back(engine::event::GamepadButtonReleased{.button = static_cast<GamepadButton>(i)});
        }
    }
    for (std::size_t i = 0; i < kGamepadAxisCount; ++i) {
        if (_gamepadAxes.at(i) != 0.0F) {
            _events.emplace_back(engine::event::GamepadAxisMoved{.axis = static_cast<GamepadAxis>(i), .value = 0.0F});
        }
    }
    _gamepadButtons.fill(false);
    _gamepadAxes.fill(0.0F);
    _gamepadId = -1;
    _events.emplace_back(engine::event::GamepadDisconnected{});
}

}  // namespace rtype::platform
