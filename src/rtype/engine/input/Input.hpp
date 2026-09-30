/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Input
*/

#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <unordered_map>

#include "engine/event/Event.hpp"
#include "engine/input/Control.hpp"
#include "engine/input/InputAction.hpp"
#include "engine/input/Key.hpp"

namespace rtype::engine::input {

/**
 * @brief Per-frame input state built from engine events, driven by named actions.
 *
 * @details Input never talks to a device or a window library: the platform layer
 * feeds it events, so it works the same with any backend and can be tested without
 * a window. Register actions once with addAction(), then every frame:
 *
 * @code
 * for (const auto& event : window.pollEvents()) {
 *     input.handleEvent(event);
 * }
 * input.update();
 * if (input.getAction("fire").isPressed()) { ... }
 * @endcode
 *
 * A key pressed and released between two frames still counts as held for one frame,
 * so short taps are never missed.
 */
class Input {
  public:
    /// @brief Feeds one event. Call it for every event of the frame, before update().
    void handleEvent(const Event& event);

    /// @brief Updates every action from the events received since the last call. Call once per frame.
    void update();

    /// @brief Creates (or replaces) an action. The returned reference stays valid.
    InputAction& addAction(const std::string& name, ActionType type);

    /// @throws InputException If no action has this name.
    [[nodiscard]] InputAction& getAction(const std::string& name);

    /// @throws InputException If no action has this name.
    [[nodiscard]] const InputAction& getAction(const std::string& name) const;

    /// @brief Deletes an action by name.
    /// @throws InputException If no action has this name.
    void removeAction(const std::string& name);

    /// @brief Deletes all actions.
    void clearActions() noexcept;

    /// @return The current value of a single control, without going through an action.
    [[nodiscard]] float read(const Control& control) const;

    /// @return Cursor position in window pixels (origin top-left).
    [[nodiscard]] glm::vec2 getMousePosition() const noexcept;

    /// @return True if a gamepad is currently connected.
    [[nodiscard]] bool isGamepadConnected() const noexcept;

    /// @brief Gamepad stick values below this are read as 0. Clamped to [0, 0.99], defaults to 0.15.
    void setDeadzone(float deadzone) noexcept;

  private:
    /**
     * @brief Held state of a family of buttons (keys, mouse buttons or gamepad buttons).
     *
     * @details Events update the live state at any time. beginFrame() then takes the
     * snapshot the frame reads: held now, or pressed at least once since the last frame.
     */
    template <typename Button>
    class ButtonStates {
      public:
        void press(Button button) {
            if (isValid(button)) {
                _down.set(index(button));
                _tapped.set(index(button));
            }
        }
        void release(Button button) {
            if (isValid(button)) {
                _down.reset(index(button));
            }
        }
        void releaseAll() noexcept { _down.reset(); }
        void beginFrame() noexcept {
            _frame = _down | _tapped;
            _tapped.reset();
        }
        [[nodiscard]] bool isDown(Button button) const { return isValid(button) && _frame.test(index(button)); }

      private:
        static constexpr std::size_t kCount = static_cast<std::size_t>(Button::kCount);

        [[nodiscard]] static constexpr std::size_t index(Button button) noexcept {
            return static_cast<std::size_t>(button);
        }
        [[nodiscard]] static constexpr bool isValid(Button button) noexcept { return index(button) < kCount; }

        std::bitset<kCount> _down;    ///< Held right now.
        std::bitset<kCount> _tapped;  ///< Pressed at least once since the last beginFrame().
        std::bitset<kCount> _frame;   ///< Snapshot read during the current frame.
    };

    /// @name Event handlers, dispatched by handleEvent().
    /// @{
    void onEvent(const event::KeyPressed& event);
    void onEvent(const event::KeyReleased& event);
    void onEvent(const event::MouseButtonPressed& event);
    void onEvent(const event::MouseButtonReleased& event);
    void onEvent(const event::MouseMoved& event) noexcept;
    void onEvent(const event::MouseScrolled& event) noexcept;
    void onEvent(const event::FocusChanged& event) noexcept;
    void onEvent(const event::GamepadConnected& event) noexcept;
    void onEvent(const event::GamepadDisconnected& event) noexcept;
    void onEvent(const event::GamepadButtonPressed& event);
    void onEvent(const event::GamepadButtonReleased& event);
    void onEvent(const event::GamepadAxisMoved& event);
    /// @brief Events Input does not use (Closed, Resized, TextEntered...).
    template <typename Unused>
    void onEvent(const Unused& /*event*/) noexcept {}
    /// @}

    [[nodiscard]] float readGamepadAxis(GamepadAxis axis) const;
    [[nodiscard]] glm::vec2 evaluate(const InputAction::Binding& binding) const;

    std::unordered_map<std::string, InputAction> _actions;  ///< Actions by name.

    ButtonStates<Key> _keys;
    ButtonStates<MouseButton> _mouseButtons;
    ButtonStates<GamepadButton> _gamepadButtons;
    std::array<float, static_cast<std::size_t>(GamepadAxis::kCount)> _gamepadAxes{};  ///< Raw values, no deadzone.
    bool _gamepadConnected = false;
    float _deadzone = 0.15F;  ///< Stick values below this are read as 0.

    glm::vec2 _mousePosition{0.0F};      ///< Last known cursor position in pixels.
    glm::vec2 _pendingMouseDelta{0.0F};  ///< Movement accumulated since the last update().
    glm::vec2 _mouseDelta{0.0F};         ///< Movement read during the current frame.
    glm::vec2 _pendingScroll{0.0F};      ///< Scroll accumulated since the last update().
    glm::vec2 _scroll{0.0F};             ///< Scroll read during the current frame.
};
}  // namespace rtype::engine::input
