/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputAction
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <vector>

#include "engine/input/Control.hpp"

namespace rtype::engine::input {

/// @brief What an action produces
enum class ActionType : std::uint8_t {
    kButton,   ///< On/off, use isPressed() / isHeld() / isReleased().
    kAxis,     ///< A single float, use readAxis().
    kVector2,  ///< A 2D vector, use readVector().
};

/**
 * @brief A named, device-independent input, e.g. "move" or "fire".
 *
 * @details An action holds any number of bindings, from any device. Each frame
 * every binding is evaluated and the one with the largest magnitude wins, so the
 * keyboard, the mouse and a gamepad can drive the same action.
 *
 * @code
 * input.addAction("move", ActionType::kVector2)
 *     .bindVector(Control::key(Key::kW), Control::key(Key::kS), Control::key(Key::kA), Control::key(Key::kD))
 *     .bindVector(Control::gamepadAxis(GamepadAxis::kLeftX), Control::gamepadAxis(GamepadAxis::kLeftY));
 * @endcode
 */
class InputAction {
  public:
    explicit InputAction(ActionType type) noexcept;

    /// @brief Binds a single control. For a kVector2 action it drives X.
    InputAction& bind(Control control);
    /// @brief 1D composite: negative gives -1, positive gives +1 (e.g. A / D).
    InputAction& bindAxis(Control negative, Control positive);
    /// @brief 2D composite from four controls (WASD, arrows, d-pad). Diagonals are normalized.
    InputAction& bindVector(Control up, Control down, Control left, Control right);
    /// @brief 2D vector from two analog controls (gamepad stick, mouse delta).
    InputAction& bindVector(Control x, Control y);

    /// @brief Magnitude above which the action counts as held. Defaults to 0.5.
    InputAction& setPressPoint(float pressPoint) noexcept;

    /// @return True only on the frame the action starts being held.
    [[nodiscard]] bool isPressed() const noexcept;
    /// @return True every frame the action is held.
    [[nodiscard]] bool isHeld() const noexcept;
    /// @return True only on the frame the action stops being held.
    [[nodiscard]] bool isReleased() const noexcept;

    /// @return The current value as a float (the X of the value).
    [[nodiscard]] float readAxis() const noexcept;
    /// @return The current value as a 2D vector.
    [[nodiscard]] glm::vec2 readVector() const noexcept;

    [[nodiscard]] ActionType getType() const noexcept;

  private:
    friend class Input;

    /// @brief One control contributing along a direction.
    struct Part {
        Control control;
        glm::vec2 direction;
    };

    /// @brief A group of parts evaluated together into one vector.
    struct Binding {
        std::vector<Part> parts;
        bool normalize = false;  ///< Clamps the result to length 1 (digital composites).
    };

    /// @brief Stores this frame's value and shifts the held state. Called by Input::update().
    void update(glm::vec2 value) noexcept;

    ActionType _type;                ///< What the action produces.
    std::vector<Binding> _bindings;  ///< Every source that can drive the action.
    glm::vec2 _value{0.0F};          ///< Value of the winning binding this frame.
    float _pressPoint = 0.5F;        ///< Magnitude above which the action is held.
    bool _held = false;              ///< Held this frame.
    bool _wasHeld = false;           ///< Held the previous frame.
};
}  // namespace rtype::engine::input
