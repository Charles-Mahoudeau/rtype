/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Control
*/

#pragma once

#include <cstdint>

#include "engine/input/Key.hpp"

namespace rtype::engine::input {

/// @brief One physical control (key, mouse button, stick axis...) read as a float.
///
/// @details Buttons read 0 or 1, analog axes read a value in [-1, 1] (triggers in [0, 1]),
/// mouse deltas read pixels and scroll reads wheel steps. Every Y axis is up-positive,
/// so that "up" on WASD, on a stick and on the mouse all mean +Y.
struct Control {
    enum class Source : std::uint8_t {
        kKey,
        kMouseButton,
        kMouseDeltaX,
        kMouseDeltaY,
        kMouseScrollX,
        kMouseScrollY,
        kGamepadButton,
        kGamepadAxis
    };

    Source source{};        ///< Device and kind of control.
    std::uint8_t code = 0;  ///< Engine Key / MouseButton / GamepadButton / GamepadAxis value, depending on source.
    float scale = 1.0F;     ///< Multiplier applied to the raw value (sensitivity, inversion).

    /// @brief Keyboard key, e.g. Control::key(Key::kW).
    [[nodiscard]] static constexpr Control key(engine::input::Key key) noexcept {
        return {.source = Source::kKey, .code = static_cast<std::uint8_t>(key)};
    }
    /// @brief Mouse button, e.g. Control::mouseButton(MouseButton::kLeft).
    [[nodiscard]] static constexpr Control mouseButton(engine::input::MouseButton button) noexcept {
        return {.source = Source::kMouseButton, .code = static_cast<std::uint8_t>(button)};
    }
    /// @brief Horizontal mouse movement since the previous frame, in pixels.
    [[nodiscard]] static constexpr Control mouseDeltaX(float sensitivity = 1.0F) noexcept {
        return {.source = Source::kMouseDeltaX, .code = 0, .scale = sensitivity};
    }
    /// @brief Vertical mouse movement since the previous frame, in pixels (up-positive).
    [[nodiscard]] static constexpr Control mouseDeltaY(float sensitivity = 1.0F) noexcept {
        return {.source = Source::kMouseDeltaY, .code = 0, .scale = sensitivity};
    }
    /// @brief Horizontal scroll since the previous frame, in wheel steps.
    [[nodiscard]] static constexpr Control mouseScrollX(float scale = 1.0F) noexcept {
        return {.source = Source::kMouseScrollX, .code = 0, .scale = scale};
    }
    /// @brief Vertical scroll since the previous frame, in wheel steps (away from the user is positive).
    [[nodiscard]] static constexpr Control mouseScrollY(float scale = 1.0F) noexcept {
        return {.source = Source::kMouseScrollY, .code = 0, .scale = scale};
    }
    /// @brief Gamepad button, e.g. Control::gamepadButton(GamepadButton::kSouth).
    [[nodiscard]] static constexpr Control gamepadButton(engine::input::GamepadButton button) noexcept {
        return {.source = Source::kGamepadButton, .code = static_cast<std::uint8_t>(button)};
    }
    /// @brief Gamepad axis, e.g. Control::gamepadAxis(GamepadAxis::kLeftX).
    [[nodiscard]] static constexpr Control gamepadAxis(engine::input::GamepadAxis axis, float scale = 1.0F) noexcept {
        return {.source = Source::kGamepadAxis, .code = static_cast<std::uint8_t>(axis), .scale = scale};
    }

    /// @return A copy of this control with its value negated.
    [[nodiscard]] constexpr Control inverted() const noexcept {
        return {.source = source, .code = code, .scale = -scale};
    }
};
}  // namespace rtype::engine::input
