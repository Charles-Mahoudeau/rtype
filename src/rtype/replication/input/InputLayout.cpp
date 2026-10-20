/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputLayout
*/

#include "rtype/replication/input/InputLayout.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <utility>

#include "rtype/replication/component/PlayerInput.hpp"
#include "rtype/replication/exceptions/ReplicationExceptions.hpp"
#include "rtype/replication/input/InputKind.hpp"

namespace rtype::replication::input {
namespace {
/// @brief The longest name the wire can carry.
constexpr std::size_t kMaxNameLength = 255;

/// @brief The most buttons a PlayerInput holds: one bit each.
constexpr std::size_t kMaxButtons = 32;

/// @return The number of axis slots an action of this kind uses.
constexpr std::size_t slotsOf(const rtype::replication::input::InputKind kind) noexcept {
    switch (kind) {
        case rtype::replication::input::InputKind::kAxis:
            return 1;
        case rtype::replication::input::InputKind::kVector2:
            return 2;
        case rtype::replication::input::InputKind::kButton:
            break;
    }
    return 0;
}

/// @brief Clamps to [-1, 1]; NaN becomes 0.
float sanitize(const float value) noexcept {
    if (std::isnan(value)) {
        return 0.0F;
    }
    return std::clamp(value, -1.0F, 1.0F);
}

/// @throws exceptions::InputLayoutException When the handle is not of the expected kind.
void requireKind(const rtype::replication::input::InputHandle handle, const rtype::replication::input::InputKind kind) {
    if (handle.kind != kind) {
        throw rtype::replication::exceptions::InputLayoutException{std::format(
            "The input action is of kind {}, not {}", static_cast<int>(handle.kind), static_cast<int>(kind))};
    }
}
}  // namespace

std::span<const InputAction> InputLayout::actions() const noexcept { return _actions; }

bool InputLayout::isFrozen() const noexcept { return _frozen; }

void InputLayout::add(std::string name, const InputKind kind) {
    if (_frozen) {
        throw exceptions::InputLayoutException{
            std::format("Cannot add the input action '{}': the layout is frozen, declare actions at startup", name)};
    }
    if (name.empty() || name.size() > kMaxNameLength) {
        throw exceptions::InputLayoutException{
            std::format("Invalid input action name of {} bytes: it must have 1 to {}", name.size(), kMaxNameLength)};
    }
    const auto position = std::ranges::lower_bound(_actions, name, std::ranges::less{}, &InputAction::name);
    if (position != _actions.end() && position->name == name) {
        throw exceptions::InputLayoutException{std::format("The input action '{}' is already declared", name)};
    }
    const std::size_t buttons = _buttonCount + (kind == InputKind::kButton ? 1 : 0);
    const std::size_t slots = _axisSlotCount + slotsOf(kind);
    if (buttons > kMaxButtons) {
        throw exceptions::InputLayoutException{
            std::format("Cannot add the input action '{}': a layout holds at most {} buttons", name, kMaxButtons)};
    }
    if (slots > component::kMaxInputAxes) {
        throw exceptions::InputLayoutException{std::format(
            "Cannot add the input action '{}': a layout holds at most {} axis slots", name, component::kMaxInputAxes)};
    }
    _actions.insert(position, InputAction{.name = std::move(name), .kind = kind});
    _buttonCount = buttons;
    _axisSlotCount = slots;
    assignHandles();
}

void InputLayout::freeze() noexcept { _frozen = true; }

std::size_t InputLayout::buttonCount() const noexcept { return _buttonCount; }

std::size_t InputLayout::axisSlotCount() const noexcept { return _axisSlotCount; }

void InputLayout::assignHandles() {
    _handles.clear();
    std::uint32_t button = 0;
    std::uint32_t slot = 0;
    for (const InputAction& action : _actions) {
        if (action.kind == InputKind::kButton) {
            _handles.push_back(InputHandle{.kind = action.kind, .index = button++});
        } else {
            _handles.push_back(InputHandle{.kind = action.kind, .index = slot});
            slot += static_cast<std::uint32_t>(slotsOf(action.kind));
        }
    }
}

InputHandle InputLayout::find(const std::string_view action) const {
    const auto position = std::ranges::lower_bound(_actions, action, std::ranges::less{}, &InputAction::name);
    if (position == _actions.end() || position->name != action) {
        throw exceptions::InputLayoutException{std::format("Unknown input action '{}'", action)};
    }
    // NOLINTNEXTLINE(*-avoid-unchecked-container-access): position is within _actions, which _handles mirrors
    return _handles[static_cast<std::size_t>(position - _actions.begin())];
}

// NOLINTBEGIN(readability-convert-member-functions-to-static,cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)

bool InputLayout::isHeld(const component::PlayerInput& input, const std::string_view action) const {
    return isHeld(input, find(action));
}

bool InputLayout::isHeld(const component::PlayerInput& input, const InputHandle action) const {
    requireKind(action, InputKind::kButton);
    return ((input.buttons >> action.index) & 1U) != 0;
}

bool InputLayout::isPressed(const component::PlayerInput& input, const std::string_view action) const {
    return isPressed(input, find(action));
}

bool InputLayout::isPressed(const component::PlayerInput& input, const InputHandle action) const {
    requireKind(action, InputKind::kButton);
    return (((input.buttons & ~input.previousButtons) >> action.index) & 1U) != 0;
}

bool InputLayout::isReleased(const component::PlayerInput& input, const std::string_view action) const {
    return isReleased(input, find(action));
}

bool InputLayout::isReleased(const component::PlayerInput& input, const InputHandle action) const {
    requireKind(action, InputKind::kButton);
    return (((~input.buttons & input.previousButtons) >> action.index) & 1U) != 0;
}

float InputLayout::readAxis(const component::PlayerInput& input, const std::string_view action) const {
    return readAxis(input, find(action));
}

float InputLayout::readAxis(const component::PlayerInput& input, const InputHandle action) const {
    requireKind(action, InputKind::kAxis);
    return input.axes[action.index];
}

std::array<float, 2> InputLayout::readVector(const component::PlayerInput& input, const std::string_view action) const {
    return readVector(input, find(action));
}

std::array<float, 2> InputLayout::readVector(const component::PlayerInput& input, const InputHandle action) const {
    requireKind(action, InputKind::kVector2);
    return {input.axes[action.index], input.axes[action.index + 1]};
}

void InputLayout::setButton(component::PlayerInput& input, const std::string_view action, const bool held) const {
    setButton(input, find(action), held);
}

void InputLayout::setButton(component::PlayerInput& input, const InputHandle action, const bool held) const {
    requireKind(action, InputKind::kButton);
    const std::uint32_t bit = 1U << action.index;
    input.buttons = held ? (input.buttons | bit) : (input.buttons & ~bit);
}

void InputLayout::setAxis(component::PlayerInput& input, const std::string_view action, const float value) const {
    setAxis(input, find(action), value);
}

void InputLayout::setAxis(component::PlayerInput& input, const InputHandle action, const float value) const {
    requireKind(action, InputKind::kAxis);
    input.axes[action.index] = sanitize(value);
}

void InputLayout::setVector(component::PlayerInput& input, const std::string_view action,
                            const std::array<float, 2> value) const {
    setVector(input, find(action), value);
}

void InputLayout::setVector(component::PlayerInput& input, const InputHandle action,
                            const std::array<float, 2> value) const {
    requireKind(action, InputKind::kVector2);
    input.axes[action.index] = sanitize(value[0]);
    input.axes[action.index + 1] = sanitize(value[1]);
}

// NOLINTEND(readability-convert-member-functions-to-static,cppcoreguidelines-pro-bounds-constant-array-index,cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
}  // namespace rtype::replication::input
