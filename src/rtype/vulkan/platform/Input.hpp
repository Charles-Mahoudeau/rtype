/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Input
*/

#pragma once

#include <GLFW/glfw3.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace rtype::vulkan::platform {

/// @brief 2D value read from an action. Y points up.
struct Vector2 {
    float x = 0.0f;
    float y = 0.0f;

    [[nodiscard]] float length() const noexcept { return std::hypot(x, y); }
};

/**
 * @brief One physical control (key, mouse button, stick axis...) read as a float.
 *
 * @details Buttons read 0 or 1, analog axes read a value in [-1, 1] (triggers in [0, 1]),
 * mouse deltas read pixels. Every Y axis is up-positive: GLFW's mouse and stick Y
 * are flipped so that "up" on WASD, on a stick and on the mouse all mean +Y.
 */
struct Control {
    enum class Source : std::uint8_t { Key, MouseButton, MouseDeltaX, MouseDeltaY, GamepadButton, GamepadAxis };

    Source source;       ///< Device and kind of control.
    int code = 0;        ///< GLFW key / button / axis code.
    float scale = 1.0f;  ///< Multiplier applied to the raw value (sensitivity, inversion).

    /// @brief Keyboard key, e.g. Control::key(GLFW_KEY_W).
    [[nodiscard]] static constexpr Control key(int key) noexcept { return {Source::Key, key}; }
    /// @brief Mouse button, e.g. Control::mouseButton(GLFW_MOUSE_BUTTON_LEFT).
    [[nodiscard]] static constexpr Control mouseButton(int button) noexcept { return {Source::MouseButton, button}; }
    /// @brief Horizontal mouse movement since the previous frame, in pixels.
    [[nodiscard]] static constexpr Control mouseDeltaX(float sensitivity = 1.0f) noexcept {
        return {Source::MouseDeltaX, 0, sensitivity};
    }
    /// @brief Vertical mouse movement since the previous frame, in pixels (up-positive).
    [[nodiscard]] static constexpr Control mouseDeltaY(float sensitivity = 1.0f) noexcept {
        return {Source::MouseDeltaY, 0, sensitivity};
    }
    /// @brief Gamepad button, e.g. Control::gamepadButton(GLFW_GAMEPAD_BUTTON_A).
    [[nodiscard]] static constexpr Control gamepadButton(int button) noexcept {
        return {Source::GamepadButton, button};
    }
    /// @brief Gamepad axis, e.g. Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X).
    [[nodiscard]] static constexpr Control gamepadAxis(int axis, float scale = 1.0f) noexcept {
        return {Source::GamepadAxis, axis, scale};
    }

    /// @return A copy of this control with its value negated.
    [[nodiscard]] constexpr Control inverted() const noexcept { return {source, code, -scale}; }
};

/// @brief What an action produces (Type of input).
enum class ActionType : std::uint8_t {
    Button,   ///< On/off, use isPressed() / isHeld() / isReleased().
    Axis,     ///< A single float, use readAxis().
    Vector2,  ///< A 2D vector, use readVector().
};

/**
 * @brief A named, device-independent input, e.g. "move" or "fire".
 *
 * @details An action holds any number of bindings, from any device. Each frame
 * every binding is evaluated and the one with the largest magnitude wins, so the
 * keyboard, the mouse and a gamepad can drive the same action.
 *
 * @code
 * input.addAction("move", ActionType::Vector2)
 *     .bindVector(Control::key(GLFW_KEY_W), Control::key(GLFW_KEY_S),
 *                 Control::key(GLFW_KEY_A), Control::key(GLFW_KEY_D))
 *     .bindVector(Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X), Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_Y));
 * @endcode
 */
class InputAction {
  public:
    explicit InputAction(ActionType type) noexcept;

    /// @brief Binds a single control. For a Vector2 action it drives X.
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
    [[nodiscard]] Vector2 readVector() const noexcept;

    [[nodiscard]] ActionType getType() const noexcept;

  private:
    friend class Input;

    /// @brief One control contributing along a direction.
    struct Part {
        Control control;
        Vector2 direction;
    };

    /// @brief A group of parts evaluated together into one Vector2.
    struct Binding {
        std::vector<Part> parts;
        bool normalize = false;  ///< Clamps the result to length 1 (digital composites).
    };

    /// @brief Stores this frame's value and shifts the held state. Called by Input::update().
    void update(Vector2 value) noexcept;

    ActionType _type;                ///< What the action produces.
    std::vector<Binding> _bindings;  ///< Every source that can drive the action.
    Vector2 _value;                  ///< Value of the winning binding this frame.
    float _pressPoint = 0.5f;        ///< Magnitude above which the action is held.
    bool _held = false;              ///< Held this frame.
    bool _wasHeld = false;           ///< Held the previous frame.
};

/**
 * @brief Per-frame input built on top of GLFW polling, driven by named actions.
 *
 * @details Register actions once with addAction(), then call update() once per
 * frame, after glfwPollEvents(). The first connected gamepad is used.
 *
 * @note Polling can miss a key pressed and released between two frames.
 * Use GLFW callbacks instead if that matters (e.g. text input).
 */
class Input {
  public:
    /// @param window GLFW window to read input from (not owned).
    explicit Input(GLFWwindow* window);

    /// @brief Reads the devices and updates every action. Call once per frame.
    void update();

    /// @brief Creates (or replaces) an action. The returned reference stays valid.
    InputAction& addAction(const std::string& name, ActionType type);
    /// @throws InputException If no action has this name.
    [[nodiscard]] InputAction& getAction(const std::string& name);
    /// @throws InputException If no action has this name.
    [[nodiscard]] const InputAction& getAction(const std::string& name) const;

    /// @return The current value of a single control, without going through an action.
    [[nodiscard]] float read(const Control& control) const;

    /// @return Cursor position in window pixels (origin top-left).
    [[nodiscard]] Vector2 getMousePosition() const noexcept;
    /// @return True if a gamepad is currently connected.
    [[nodiscard]] bool isGamepadConnected() const noexcept;

    /// @brief Gamepad stick values below this are read as 0. Defaults to 0.15.
    void setDeadzone(float deadzone) noexcept;
    /// @brief Hides and locks the cursor, useful for an FPS camera.
    void setCursorLocked(bool locked);

  private:
    void updateMouse();
    void updateGamepad();
    [[nodiscard]] float readGamepadAxis(int axis) const;
    [[nodiscard]] Vector2 evaluate(const InputAction::Binding& binding) const;

    GLFWwindow* _window;                                   ///< GLFW window to read input from (not owned).
    std::unordered_map<std::string, InputAction> _actions;  ///< Actions by name.

    GLFWgamepadstate _gamepad{};    ///< State of the first connected gamepad.
    bool _gamepadConnected = false;  ///< True if _gamepad holds a valid state.
    float _deadzone = 0.15f;         ///< Stick values below this are read as 0.

    double _mouseX = 0.0;       ///< Current mouse X position in pixels.
    double _mouseY = 0.0;       ///< Current mouse Y position in pixels.
    double _mouseDeltaX = 0.0;  ///< Mouse movement since the previous frame, in pixels.
    double _mouseDeltaY = 0.0;  ///< Mouse movement since the previous frame, in pixels.
    bool _firstMouse = true;    ///< True until the first read, or after locking the cursor, to avoid a jump.
};
}  // namespace rtype::vulkan::platform
