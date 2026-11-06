/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ControlNames
*/

#pragma once

#include <optional>
#include <string_view>

#include "plugins/input/Control.hpp"
#include "plugins/input/Export.hpp"

namespace rtype::plugins::input {

/// @brief Finds a control by its name, as scripts and configuration files write it:
///
/// | Name                                   | Control                                     |
/// |----------------------------------------|---------------------------------------------|
/// | a key: "W", "Space", "Up", "LeftShift" | Control::key() (contract::platform::input::keyFromName()) |
/// | "Mouse" + a button: "MouseLeft"        | Control::mouseButton()                      |
/// | "MouseDeltaX", "MouseDeltaY"           | Control::mouseDeltaX(), mouseDeltaY()       |
/// | "MouseScrollX", "MouseScrollY"         | Control::mouseScrollX(), mouseScrollY()     |
/// | "Gamepad" + a button: "GamepadSouth"   | Control::gamepadButton()                    |
/// | "Gamepad" + an axis: "GamepadLeftX"    | Control::gamepadAxis()                      |
///
/// @return The control, with a scale of 1, or nothing if no control has this name (names are case-sensitive).
[[nodiscard]] RTYPE_PLUGIN_INPUT_API std::optional<Control> controlFromName(std::string_view name) noexcept;

}  // namespace rtype::plugins::input
