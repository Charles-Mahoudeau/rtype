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
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <string>
#include <utility>
#include <variant>

#include "engine/event/Event.hpp"
#include "engine/input/Control.hpp"
#include "engine/input/Key.hpp"
#include "plugins/input/InputAction.hpp"
#include "plugins/input/exceptions/InputExceptions.hpp"

namespace rtype::plugins::input {

void Input::handleEvent(const engine::Event& event) {
    std::visit([this](const auto& data) { onEvent(data); }, event);
}

void Input::onEvent(const engine::event::KeyPressed& event) {
    if (!event.repeat && event.key != engine::input::Key::kUnknown) {
        _keys.press(event.key);
    }
}

void Input::onEvent(const engine::event::KeyReleased& event) { _keys.release(event.key); }

void Input::onEvent(const engine::event::MouseButtonPressed& event) {
    if (event.button != engine::input::MouseButton::kUnknown) {
        _mouseButtons.press(event.button);
    }
}

void Input::onEvent(const engine::event::MouseButtonReleased& event) { _mouseButtons.release(event.button); }

void Input::onEvent(const engine::event::MouseMoved& event) noexcept {
    _mousePosition = event.position;
    _pendingMouseDelta += event.delta;
}

void Input::onEvent(const engine::event::MouseScrolled& event) noexcept { _pendingScroll += event.offset; }

void Input::onEvent(const engine::event::FocusChanged& event) noexcept {
    if (!event.focused) {
        _keys.releaseAll();
        _mouseButtons.releaseAll();
    }
}

void Input::onEvent(const engine::event::GamepadConnected& /*event*/) noexcept { _gamepadConnected = true; }

void Input::onEvent(const engine::event::GamepadDisconnected& /*event*/) noexcept {
    _gamepadConnected = false;
    _gamepadButtons.releaseAll();
    _gamepadAxes.fill(0.0F);
}

void Input::onEvent(const engine::event::GamepadButtonPressed& event) { _gamepadButtons.press(event.button); }

void Input::onEvent(const engine::event::GamepadButtonReleased& event) { _gamepadButtons.release(event.button); }

void Input::onEvent(const engine::event::GamepadAxisMoved& event) {
    if (event.axis < engine::input::GamepadAxis::kCount) {
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

float Input::read(const engine::input::Control& control) const {
    switch (control.source) {
        case engine::input::Control::Source::kKey:
            return _keys.isDown(static_cast<engine::input::Key>(control.code)) ? control.scale : 0.0F;
        case engine::input::Control::Source::kMouseButton:
            return _mouseButtons.isDown(static_cast<engine::input::MouseButton>(control.code)) ? control.scale : 0.0F;
        case engine::input::Control::Source::kMouseDeltaX:
            return _mouseDelta.x * control.scale;
        case engine::input::Control::Source::kMouseDeltaY:
            return -_mouseDelta.y * control.scale;
        case engine::input::Control::Source::kMouseScrollX:
            return _scroll.x * control.scale;
        case engine::input::Control::Source::kMouseScrollY:
            return _scroll.y * control.scale;
        case engine::input::Control::Source::kGamepadButton:
            return _gamepadButtons.isDown(static_cast<engine::input::GamepadButton>(control.code)) ? control.scale
                                                                                                   : 0.0F;
        case engine::input::Control::Source::kGamepadAxis:
            return readGamepadAxis(static_cast<engine::input::GamepadAxis>(control.code)) * control.scale;
    }
    return 0.0F;
}

float Input::readGamepadAxis(engine::input::GamepadAxis axis) const {
    if (!_gamepadConnected || axis >= engine::input::GamepadAxis::kCount) {
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
        throw exceptions::InputException(std::format("Unknown input action: {}", name));
    }
    _actions.erase(it);
}

void Input::clearActions() noexcept { _actions.clear(); }

InputAction& Input::getAction(const std::string& name) {
    auto it = _actions.find(name);
    if (it == _actions.end()) {
        throw exceptions::InputException(std::format("Unknown input action: {}", name));
    }
    return it->second;
}

const InputAction& Input::getAction(const std::string& name) const {
    auto it = _actions.find(name);
    if (it == _actions.end()) {
        throw exceptions::InputException(std::format("Unknown input action: {}", name));
    }
    return it->second;
}

glm::vec2 Input::getMousePosition() const noexcept { return _mousePosition; }

bool Input::isGamepadConnected() const noexcept { return _gamepadConnected; }

void Input::setDeadzone(float deadzone) noexcept {
    _deadzone = std::isfinite(deadzone) ? std::clamp(deadzone, 0.0F, 0.99F) : 0.0F;
}

}  // namespace rtype::plugins::input
