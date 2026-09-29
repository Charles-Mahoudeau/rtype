/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Input
*/

#pragma once

#include <GLFW/glfw3.h>

#include <glm/ext/vector_double2.hpp>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <unordered_map>

#include "platform/input/Control.hpp"
#include "platform/input/InputAction.hpp"

namespace rtype::vulkan::platform::input {

/**
 * @brief Per-frame input built on top of GLFW polling, driven by named actions.
 *
 * @details Register actions once with addAction(), then call update() once per
 * frame, after glfwPollEvents(). The first connected gamepad is used.
 *
 * @note Polling can miss a key pressed and released between two frames.
 * Use GLFW callbacks instead if that matters (e.g. text input).
 *
 * @warning On macOS 11.3+, controllers natively handled by Apple's GameController
 * framework (e.g. Switch Pro Controller) are detected by GLFW but never send updates,
 * so they read as idle. Gamepads are not supported on macOS for now.
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

    /// @brief Deletes an action by name. Does nothing if no action has this name.
    /// @throws InputException If no action has this name.
    void removeAction(const std::string& name);

    /// @brief Deletes all actions.
    /// @note This is not necessary, as the destructor will do it automatically.
    void clearActions() noexcept;

    /// @throws InputException If no action has this name.
    [[nodiscard]] const InputAction& getAction(const std::string& name) const;

    /// @return The current value of a single control, without going through an action.
    [[nodiscard]] float read(const Control& control) const;

    /// @return Cursor position in window pixels (origin top-left).
    [[nodiscard]] glm::vec2 getMousePosition() const noexcept;
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
    [[nodiscard]] glm::vec2 evaluate(const InputAction::Binding& binding) const;

    GLFWwindow* _window;                                    ///< GLFW window to read input from (not owned).
    std::unordered_map<std::string, InputAction> _actions;  ///< Actions by name.

    GLFWgamepadstate _gamepad{};     ///< State of the first connected gamepad.
    bool _gamepadConnected = false;  ///< True if _gamepad holds a valid state.
    float _deadzone = 0.15F;         ///< Stick values below this are read as 0.

    glm::dvec2 _mousePosition{0.0};  ///< Current mouse position in pixels.
    glm::dvec2 _mouseDelta{0.0};     ///< Mouse movement since the previous frame, in pixels.
    bool _firstMouse = true;         ///< True until the first read, or after locking the cursor, to avoid a jump.
};
}  // namespace rtype::vulkan::platform::input
