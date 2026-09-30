/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Input
*/

#include "Input.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <string>
#include <utility>
#include <variant>

#include "engine/event/Event.hpp"
#include "engine/exceptions/InputExceptions.hpp"
#include "engine/input/Control.hpp"
#include "engine/input/InputAction.hpp"
#include "engine/input/Key.hpp"

namespace rtype::engine::input {

void Input::handleEvent(const Event& event) {
    std::visit([this](const auto& data) { onEvent(data); }, event);
}

void Input::onEvent(const event::KeyPressed& event) {
    if (!event.repeat && event.key != Key::Unknown) {
        _keys.press(event.key);
    }
}

void Input::onEvent(const event::KeyReleased& event) { _keys.release(event.key); }

void Input::onEvent(const event::MouseButtonPressed& event) {
    if (event.button != MouseButton::Unknown) {
        _mouseButtons.press(event.button);
    }
}

void Input::onEvent(const event::MouseButtonReleased& event) { _mouseButtons.release(event.button); }

void Input::onEvent(const event::MouseMoved& event) noexcept {
    _mousePosition = event.position;
    _pendingMouseDelta += event.delta;
}

void Input::onEvent(const event::MouseScrolled& event) noexcept { _pendingScroll += event.offset; }

void Input::onEvent(const event::FocusChanged& event) noexcept {
    if (!event.focused) {
        _keys.releaseAll();
        _mouseButtons.releaseAll();
    }
}

void Input::onEvent(const event::GamepadConnected& /*event*/) noexcept { _gamepadConnected = true; }

void Input::onEvent(const event::GamepadDisconnected& /*event*/) noexcept {
    _gamepadConnected = false;
    _gamepadButtons.releaseAll();
    _gamepadAxes.fill(0.0F);
}

void Input::onEvent(const event::GamepadButtonPressed& event) { _gamepadButtons.press(event.button); }

void Input::onEvent(const event::GamepadButtonReleased& event) { _gamepadButtons.release(event.button); }

void Input::onEvent(const event::GamepadAxisMoved& event) {
    if (event.axis < GamepadAxis::Count) {
        _gamepadAxes.at(static_cast<std::size_t>(event.axis)) = event.value;
    }
}

void Input::update() {
    _keys.beginFrame();
    _mouseButtons.beginFrame();
    _gamepadButtons.beginFrame();
    _mouseDelta = std::exchange(_pendingMouseDelta, glm::vec2{0.0F});
    _scroll = std::exchange(_pendingScroll, glm::vec2{0.0F});

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
            return _keys.isDown(static_cast<Key>(control.code)) ? control.scale : 0.0F;
        case Control::Source::MouseButton:
            return _mouseButtons.isDown(static_cast<MouseButton>(control.code)) ? control.scale : 0.0F;
        case Control::Source::MouseDeltaX:
            return _mouseDelta.x * control.scale;
        case Control::Source::MouseDeltaY:
            return -_mouseDelta.y * control.scale;
        case Control::Source::MouseScrollX:
            return _scroll.x * control.scale;
        case Control::Source::MouseScrollY:
            return _scroll.y * control.scale;
        case Control::Source::GamepadButton:
            return _gamepadButtons.isDown(static_cast<GamepadButton>(control.code)) ? control.scale : 0.0F;
        case Control::Source::GamepadAxis:
            return readGamepadAxis(static_cast<GamepadAxis>(control.code)) * control.scale;
    }
    return 0.0F;
}

float Input::readGamepadAxis(GamepadAxis axis) const {
    if (!_gamepadConnected || axis >= GamepadAxis::Count) {
        return 0.0F;
    }
    const float value = _gamepadAxes.at(static_cast<std::size_t>(axis));
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

void Input::setDeadzone(float deadzone) noexcept {
    _deadzone = std::isfinite(deadzone) ? std::clamp(deadzone, 0.0F, 0.99F) : 0.0F;
}

}  // namespace rtype::engine::input
