/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputAction
*/

#include "InputAction.hpp"

#include <glm/geometric.hpp>

namespace rtype::vulkan::platform::input {

InputAction::InputAction(ActionType type) noexcept : _type(type) {}

InputAction& InputAction::bind(Control control) {
    _bindings.push_back({{{control, {1.0f, 0.0f}}}});
    return *this;
}

InputAction& InputAction::bindAxis(Control negative, Control positive) {
    _bindings.push_back({{{negative, {-1.0f, 0.0f}}, {positive, {1.0f, 0.0f}}}});
    return *this;
}

InputAction& InputAction::bindVector(Control up, Control down, Control left, Control right) {
    _bindings.push_back(
        {{{up, {0.0f, 1.0f}}, {down, {0.0f, -1.0f}}, {left, {-1.0f, 0.0f}}, {right, {1.0f, 0.0f}}}, true});
    return *this;
}

InputAction& InputAction::bindVector(Control x, Control y) {
    _bindings.push_back({{{x, {1.0f, 0.0f}}, {y, {0.0f, 1.0f}}}});
    return *this;
}

InputAction& InputAction::setPressPoint(float pressPoint) noexcept {
    _pressPoint = pressPoint;
    return *this;
}

void InputAction::update(glm::vec2 value) noexcept {
    _value = value;
    _wasHeld = _held;
    _held = glm::length(value) >= _pressPoint;
}

bool InputAction::isPressed() const noexcept { return _held && !_wasHeld; }

bool InputAction::isHeld() const noexcept { return _held; }

bool InputAction::isReleased() const noexcept { return !_held && _wasHeld; }

float InputAction::readAxis() const noexcept { return _value.x; }

glm::vec2 InputAction::readVector() const noexcept { return _value; }

ActionType InputAction::getType() const noexcept { return _type; }

}  // namespace rtype::vulkan::platform::input
