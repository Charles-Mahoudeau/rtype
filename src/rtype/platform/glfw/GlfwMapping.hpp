/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GlfwMapping
*/

#pragma once

#include "plugins/backend/input/Key.hpp"

/// @brief Translation between GLFW codes and the engine input types.
///
/// @details This is the only place that knows both. GLFW codes never leave the
/// platform layer: everything is translated into engine types before being sent
/// as an event.
namespace rtype::platform::glfw {

/// @return The engine key for a GLFW key code, or Key::kUnknown.
[[nodiscard]] plugins::backend::input::Key fromGlfwKey(int key);
/// @return The GLFW key code for an engine key, or GLFW_KEY_UNKNOWN.
[[nodiscard]] int toGlfwKey(plugins::backend::input::Key key);

/// @return The engine mouse button for a GLFW button code, or MouseButton::kUnknown.
[[nodiscard]] plugins::backend::input::MouseButton fromGlfwMouseButton(int button) noexcept;
/// @return The GLFW button code for an engine mouse button, or -1 for MouseButton::kUnknown.
[[nodiscard]] int toGlfwMouseButton(plugins::backend::input::MouseButton button) noexcept;

/// @return The GLFW gamepad button index for an engine gamepad button.
[[nodiscard]] int toGlfwGamepadButton(plugins::backend::input::GamepadButton button) noexcept;
/// @return The GLFW gamepad axis index for an engine gamepad axis.
[[nodiscard]] int toGlfwGamepadAxis(plugins::backend::input::GamepadAxis axis) noexcept;

/// @return A raw GLFW axis value converted to the engine convention: stick Y up-positive, triggers in [0, 1].
[[nodiscard]] float fromGlfwGamepadAxisValue(plugins::backend::input::GamepadAxis axis, float value) noexcept;

/// @return The engine modifiers for a GLFW modifier bit field.
[[nodiscard]] plugins::backend::input::Mods fromGlfwMods(int mods) noexcept;

}  // namespace rtype::platform::glfw
