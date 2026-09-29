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

namespace rtype::vulkan::platform::event {
/// @brief Represents a closed event.
/// @details This event is triggered when the window is closed.
struct Closed {};

/// @brief Represents a resized event.
/// @details This event is triggered when the window is resized.
/// @note The width and height are provided in pixels.
struct Resized {
    std::uint16_t width;
    std::uint16_t height;
};
/// @brief Represents a focus changed event.
/// @details This event is triggered when the window gains or loses focus.
struct FocusChanged {
    bool focused;
};
/// @brief Represents a key pressed event.
/// @details This event is triggered when a key is pressed.
struct KeyPressed {
    int key;
    int mods;
    bool repeat;
};

/// @brief Represents a key released event.
/// @details This event is triggered when a key is released.
struct KeyReleased {
    int key;
    int mods;
};

/// @brief Represents a text entered event.
/// @details This event is triggered when text is entered.
struct TextEntered {
    char32_t codepoint;
};

/// @brief Represents a mouse button pressed event.
/// @details This event is triggered when a mouse button is pressed.
struct MouseButtonPressed {
    int button;
    int mods;
};

/// @brief Represents a mouse button released event.
/// @details This event is triggered when a mouse button is released.
struct MouseButtonReleased {
    int button;
    int mods;
};

/// @brief Represents a mouse scrolled event.
/// @details This event is triggered when the mouse wheel is scrolled.
struct MouseScrolled {
    glm::vec2 offset;
};
}  // namespace rtype::vulkan::platform::event

namespace rtype::vulkan::platform {
/**
 * @brief A variant type representing all possible events.
 * @details This type can hold any of the event types defined in the `event` namespace.
 * It is used to represent events in a type-safe manner, allowing for easy handling of different event types.
 * @note The `Event` type is a `std::variant` that can hold any of the event types defined in the `event` namespace. It
 * is used to represent events in a type-safe manner, allowing for easy handling of different event types.
 */
using Event =
    std::variant<event::Closed, event::Resized, event::FocusChanged, event::KeyPressed, event::KeyReleased,
                 event::TextEntered, event::MouseButtonPressed, event::MouseButtonReleased, event::MouseScrolled>;
};  // namespace rtype::vulkan::platform
