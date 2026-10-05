/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputAction
*/

#include "InputAction.hpp"

#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>

#include "engine/input/Control.hpp"

namespace rtype::engine::input {

InputAction::InputAction(ActionType type) noexcept : _type(type) {}

InputAction& InputAction::bind(Control control) {
    _bindings.push_back({.parts = {{.control = control, .direction = {1.0F, 0.0F}}}});
    return *this;
}

InputAction& InputAction::bindAxis(Control negative, Control positive) {
    _bindings.push_back({.parts = {{.control = negative, .direction = {-1.0F, 0.0F}},
                                   {.control = positive, .direction = {1.0F, 0.0F}}}});
    return *this;
}

InputAction& InputAction::bindVector(Control up, Control down, Control left, Control right) {
    _bindings.push_back({.parts = {{.control = up, .direction = {0.0F, 1.0F}},
                                   {.control = down, .direction = {0.0F, -1.0F}},
                                   {.control = left, .direction = {-1.0F, 0.0F}},
                                   {.control = right, .direction = {1.0F, 0.0F}}},
                         .normalize = true});
    return *this;
}

InputAction& InputAction::bindVector(Control x, Control y) {
    _bindings.push_back(
        {.parts = {{.control = x, .direction = {1.0F, 0.0F}}, {.control = y, .direction = {0.0F, 1.0F}}}});
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

}  // namespace rtype::engine::input
