/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Control
*/

#pragma once

#include <cstdint>

namespace rtype::vulkan::platform::input {

/**
 * @brief One physical control (key, mouse button, stick axis...) read as a float.
 *
 * @details Buttons read 0 or 1, analog axes read a value in [-1, 1] (triggers in [0, 1]),
 * mouse deltas read pixels. Every Y axis is up-positive: GLFW's mouse and stick Y
 * are flipped so that "up" on WASD, on a stick and on the mouse all mean +Y.
 */
struct Control {
    enum class Source : std::uint8_t { Key, MouseButton, MouseDeltaX, MouseDeltaY, GamepadButton, GamepadAxis };

    Source source{};     ///< Device and kind of control.
    int code = 0;        ///< GLFW key / button / axis code.
    float scale = 1.0F;  ///< Multiplier applied to the raw value (sensitivity, inversion).

    /// @brief Keyboard key, e.g. Control::key(GLFW_KEY_W).
    [[nodiscard]] static constexpr Control key(int key) noexcept { return {.source = Source::Key, .code = key}; }
    /// @brief Mouse button, e.g. Control::mouseButton(GLFW_MOUSE_BUTTON_LEFT).
    [[nodiscard]] static constexpr Control mouseButton(int button) noexcept {
        return {.source = Source::MouseButton, .code = button};
    }
    /// @brief Horizontal mouse movement since the previous frame, in pixels.
    [[nodiscard]] static constexpr Control mouseDeltaX(float sensitivity = 1.0F) noexcept {
        return {.source = Source::MouseDeltaX, .code = 0, .scale = sensitivity};
    }
    /// @brief Vertical mouse movement since the previous frame, in pixels (up-positive).
    [[nodiscard]] static constexpr Control mouseDeltaY(float sensitivity = 1.0F) noexcept {
        return {.source = Source::MouseDeltaY, .code = 0, .scale = sensitivity};
    }
    /// @brief Gamepad button, e.g. Control::gamepadButton(GLFW_GAMEPAD_BUTTON_A).
    [[nodiscard]] static constexpr Control gamepadButton(int button) noexcept {
        return {.source = Source::GamepadButton, .code = button};
    }
    /// @brief Gamepad axis, e.g. Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X).
    [[nodiscard]] static constexpr Control gamepadAxis(int axis, float scale = 1.0F) noexcept {
        return {.source = Source::GamepadAxis, .code = axis, .scale = scale};
    }

    /// @return A copy of this control with its value negated.
    [[nodiscard]] constexpr Control inverted() const noexcept {
        return {.source = source, .code = code, .scale = -scale};
    }
};
}  // namespace rtype::vulkan::platform::input
