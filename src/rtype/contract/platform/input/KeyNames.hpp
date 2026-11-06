/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** KeyNames
*/

#pragma once

#include <optional>
#include <string_view>

#include "rtype/contract/platform/input/Key.hpp"

namespace rtype::contract::platform::input {

/// @brief Finds a key by its name: its enumerator without the `k`, as in "W", "Space", "Num1", "Up", "LeftShift",
/// "F5" or "KeypadEnter". Scripts and configuration files name keys this way.
/// @return The key, or nothing if no key has this name (names are case-sensitive; kUnknown has none).
[[nodiscard]] std::optional<Key> keyFromName(std::string_view name) noexcept;

/// @brief Finds a mouse button by its name: "Left", "Right", "Middle", "X1" or "X2".
/// @return The button, or nothing if no button has this name.
[[nodiscard]] std::optional<MouseButton> mouseButtonFromName(std::string_view name) noexcept;

/// @brief Finds a gamepad button by its name: its enumerator without the `k`, as in "South", "LeftBumper", "Start"
/// or "DpadUp".
/// @return The button, or nothing if no button has this name.
[[nodiscard]] std::optional<GamepadButton> gamepadButtonFromName(std::string_view name) noexcept;

/// @brief Finds a gamepad axis by its name: "LeftX", "LeftY", "RightX", "RightY", "LeftTrigger" or "RightTrigger".
/// @return The axis, or nothing if no axis has this name.
[[nodiscard]] std::optional<GamepadAxis> gamepadAxisFromName(std::string_view name) noexcept;

}  // namespace rtype::contract::platform::input
