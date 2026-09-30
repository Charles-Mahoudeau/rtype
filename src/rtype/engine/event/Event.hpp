/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Event
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <variant>

#include "engine/input/Key.hpp"

/**
 * @brief Events produced by the platform layer and consumed by the engine.
 *
 * @details The platform translates its native events (GLFW callbacks, gamepad
 * polling...) into these types, so nothing outside the platform ever sees a
 * GLFW code. Values follow the engine conventions: stick Y is up-positive and
 * triggers read [0, 1].
 */
namespace rtype::engine::event {

/// @brief The user asked to close the window.
struct Closed {};

/// @brief The framebuffer was resized, in pixels.
struct Resized {
    std::uint16_t width = 0;
    std::uint16_t height = 0;
};

/// @brief The window gained or lost the focus.
struct FocusChanged {
    bool focused = false;
};

/// @brief A key was pressed, or is being auto-repeated by the OS.
struct KeyPressed {
    input::Key key = input::Key::kUnknown;
    input::Mods mods{};
    bool repeat = false;  ///< True when the OS auto-repeats a held key.
};

/// @brief A key was released.
struct KeyReleased {
    input::Key key = input::Key::kUnknown;
    input::Mods mods{};
};

/// @brief A character was typed, with the user's keyboard layout applied.
struct TextEntered {
    char32_t codepoint = 0;
};

/// @brief The cursor moved.
struct MouseMoved {
    glm::vec2 position{0.0F};  ///< Position in window pixels, origin top-left.
    glm::vec2 delta{0.0F};     ///< Movement since the previous MouseMoved, in pixels (Y down).
};

/// @brief A mouse button was pressed.
struct MouseButtonPressed {
    input::MouseButton button = input::MouseButton::kUnknown;
    input::Mods mods{};
};

/// @brief A mouse button was released.
struct MouseButtonReleased {
    input::MouseButton button = input::MouseButton::kUnknown;
    input::Mods mods{};
};

/// @brief The mouse wheel or the touchpad scrolled.
struct MouseScrolled {
    glm::vec2 offset{0.0F};  ///< Scroll amount, Y positive away from the user.
};

/// @brief A gamepad was connected. Only the first gamepad is reported.
struct GamepadConnected {};

/// @brief The gamepad was disconnected. Every button and axis is released beforehand.
struct GamepadDisconnected {};

/// @brief A gamepad button was pressed.
struct GamepadButtonPressed {
    input::GamepadButton button = input::GamepadButton::kSouth;
};

/// @brief A gamepad button was released.
struct GamepadButtonReleased {
    input::GamepadButton button = input::GamepadButton::kSouth;
};

/// @brief A gamepad axis changed. Sticks read [-1, 1] (Y up-positive), triggers read [0, 1], no deadzone applied.
struct GamepadAxisMoved {
    input::GamepadAxis axis = input::GamepadAxis::kLeftX;
    float value = 0.0F;
};

}  // namespace rtype::engine::event

namespace rtype::engine {

/**
 * @brief Any event, read with std::get_if or std::visit.
 *
 * @code
 * for (const auto& event : window.pollEvents()) {
 *     if (const auto* key = std::get_if<event::KeyPressed>(&event)) { ... }
 * }
 * @endcode
 */
using Event = std::variant<event::Closed, event::Resized, event::FocusChanged, event::KeyPressed, event::KeyReleased,
                           event::TextEntered, event::MouseMoved, event::MouseButtonPressed, event::MouseButtonReleased,
                           event::MouseScrolled, event::GamepadConnected, event::GamepadDisconnected,
                           event::GamepadButtonPressed, event::GamepadButtonReleased, event::GamepadAxisMoved>;

}  // namespace rtype::engine
