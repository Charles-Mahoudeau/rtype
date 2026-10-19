/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Input
*/

#pragma once

#include <array>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <unordered_map>

#include "engine/event/Event.hpp"
#include "engine/input/Key.hpp"
#include "plugins/input/ButtonStates.hpp"
#include "plugins/input/Control.hpp"
#include "plugins/input/InputAction.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::input {

/// @brief Per-frame input state built from engine events, driven by named actions.
///
/// @details Input never talks to a device or a window library: the platform layer
/// feeds it events, so it works the same with any backend and can be tested without
/// a window. Register actions once with addAction(), then every frame:
///
/// @code
/// for (const auto& event : window.pollEvents()) {
///     input.handleEvent(event);
/// }
/// input.update();
/// if (input.getAction("fire").isPressed()) { ... }
/// @endcode
///
/// A key pressed and released between two frames still counts as held for one frame,
/// so short taps are never missed.
class Input {
  public:
    Input() = default;
    ~Input() = default;
    Input(const Input& other) = default;
    Input& operator=(const Input& other) = default;
    Input(Input&& other) noexcept = default;
    Input& operator=(Input&& other) noexcept = default;

    /// @brief Feeds one event. Call it for every event of the frame, before update().
    void handleEvent(const engine::Event& event);

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
    [[nodiscard]] float read(const plugins::input::Control& control) const;

    /// @return Cursor position in window pixels (origin top-left).
    [[nodiscard]] glm::vec2 getMousePosition() const noexcept;

    /// @return True if a gamepad is currently connected.
    [[nodiscard]] bool isGamepadConnected() const noexcept;

    /// @brief Gamepad stick values below this are read as 0. Clamped to [0, 0.99], defaults to 0.15.
    void setDeadzone(float deadzone) noexcept;

  private:
    /// @name Event handlers, dispatched by handleEvent().
    /// @{
    void onEvent(const engine::event::KeyPressed& event);
    void onEvent(const engine::event::KeyReleased& event);
    void onEvent(const engine::event::MouseButtonPressed& event);
    void onEvent(const engine::event::MouseButtonReleased& event);
    void onEvent(const engine::event::MouseMoved& event) noexcept;
    void onEvent(const engine::event::MouseScrolled& event) noexcept;
    void onEvent(const engine::event::FocusChanged& event) noexcept;
    void onEvent(const engine::event::GamepadConnected& event) noexcept;
    void onEvent(const engine::event::GamepadDisconnected& event) noexcept;
    void onEvent(const engine::event::GamepadButtonPressed& event);
    void onEvent(const engine::event::GamepadButtonReleased& event);
    void onEvent(const engine::event::GamepadAxisMoved& event);
    /// @brief Events Input does not use (Closed, Resized, TextEntered...).
    template <typename Unused>
    void onEvent(const Unused& event) noexcept;
    /// @}

    [[nodiscard]] float readGamepadAxis(engine::input::GamepadAxis axis) const;
    [[nodiscard]] glm::vec2 evaluate(const InputAction::Binding& binding) const;

    std::unordered_map<std::string, InputAction> _actions;  ///< Actions by name.

    ButtonStates<engine::input::Key> _keys;
    ButtonStates<engine::input::MouseButton> _mouseButtons;
    ButtonStates<engine::input::GamepadButton> _gamepadButtons;
    std::array<float, static_cast<std::size_t>(engine::input::GamepadAxis::kCount)>
        _gamepadAxes{};  ///< Raw values, no deadzone.
    bool _gamepadConnected = false;
    float _deadzone = 0.15F;  ///< Stick values below this are read as 0.

    glm::vec2 _mousePosition{0.0F};      ///< Last known cursor position in pixels.
    glm::vec2 _pendingMouseDelta{0.0F};  ///< Movement accumulated since the last update().
    glm::vec2 _mouseDelta{0.0F};         ///< Movement read during the current frame.
    glm::vec2 _pendingScroll{0.0F};      ///< Scroll accumulated since the last update().
    glm::vec2 _scroll{0.0F};             ///< Scroll read during the current frame.
};

/// @brief Input is a resource of the world, inserted by the InputPlugin.
RTYPE_ECS_RESOURCE(Input, "rtype.Input");
}  // namespace rtype::plugins::input

#include "Input.tpp"
