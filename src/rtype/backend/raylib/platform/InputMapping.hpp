/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputMapping
*/

#pragma once

#include <span>

#include "rtype/contract/platform/input/Key.hpp"

/// What raylib calls each key, mouse button and gamepad button, next to the engine's name.
namespace rtype::backend::raylib::platform {

/// @brief A raylib key and the engine key at the same place.
struct KeyPair {
    int raylib;                          ///< KEY_* of raylib.
    contract::platform::input::Key key;  ///< The engine's key.
};

/// @brief A raylib mouse button and the engine's.
struct MousePair {
    int raylib;                                     ///< MOUSE_BUTTON_* of raylib.
    contract::platform::input::MouseButton button;  ///< The engine's button.
};

/// @brief A raylib gamepad button and the engine's.
struct PadPair {
    int raylib;                                       ///< GAMEPAD_BUTTON_* of raylib.
    contract::platform::input::GamepadButton button;  ///< The engine's button.
};

/// @brief The gamepad reported: the first one.
inline constexpr int kGamepad = 0;

/// @return Every key the engine knows, with its raylib name.
[[nodiscard]] std::span<const KeyPair> keys();

/// @return Every mouse button the engine knows, with its raylib name.
[[nodiscard]] std::span<const MousePair> mouseButtons();

/// @return Every gamepad button the engine knows, with its raylib name.
[[nodiscard]] std::span<const PadPair> gamepadButtons();

/// @return The modifier keys held now.
[[nodiscard]] contract::platform::input::Mods currentMods();

/// @return The engine's value of a gamepad axis of kGamepad: sticks Y up-positive, triggers in [0, 1].
[[nodiscard]] float readAxis(contract::platform::input::GamepadAxis axis);

}  // namespace rtype::backend::raylib::platform
