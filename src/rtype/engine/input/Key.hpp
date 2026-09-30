/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Key
*/

#pragma once

#include <cstdint>

namespace rtype::engine::input {

/**
 * @brief A physical keyboard key.
 *
 * @details Keys are named after their position on a US QWERTY layout, not after the
 * character they type: Key::W is the key right of Tab on every layout (Z on AZERTY).
 * This keeps bindings like WASD in the same place for everyone. Use the TextEntered
 * event to read typed characters.
 */
enum class Key : std::uint8_t {
    Unknown,  ///< A key the engine does not know (media keys, vendor keys...).

    Space,
    Apostrophe,
    Comma,
    Minus,
    Period,
    Slash,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    Semicolon,
    Equal,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    LeftBracket,
    Backslash,
    RightBracket,
    Grave,

    Escape,
    Enter,
    Tab,
    Backspace,
    Insert,
    Delete,
    Right,
    Left,
    Down,
    Up,
    PageUp,
    PageDown,
    Home,
    End,
    CapsLock,
    ScrollLock,
    NumLock,
    PrintScreen,
    Pause,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    Keypad0,
    Keypad1,
    Keypad2,
    Keypad3,
    Keypad4,
    Keypad5,
    Keypad6,
    Keypad7,
    Keypad8,
    Keypad9,
    KeypadDecimal,
    KeypadDivide,
    KeypadMultiply,
    KeypadSubtract,
    KeypadAdd,
    KeypadEnter,
    KeypadEqual,

    LeftShift,
    LeftControl,
    LeftAlt,
    LeftSuper,
    RightShift,
    RightControl,
    RightAlt,
    RightSuper,
    Menu,

    Count  ///< Number of keys, not a key. Useful to size lookup tables.
};

/// @brief A mouse button.
enum class MouseButton : std::uint8_t {
    Unknown,  ///< A button the engine does not know.
    Left,
    Right,
    Middle,
    X1,  ///< First side button, usually "back".
    X2,  ///< Second side button, usually "forward".
    Count
};

/**
 * @brief A gamepad button, named by position so that it means the same on every brand.
 *
 * @details South is A on Xbox, Cross on PlayStation and B on Nintendo.
 */
enum class GamepadButton : std::uint8_t {
    South,
    East,
    West,
    North,
    LeftBumper,
    RightBumper,
    Back,
    Start,
    Guide,
    LeftThumb,   ///< Pressing the left stick.
    RightThumb,  ///< Pressing the right stick.
    DpadUp,
    DpadRight,
    DpadDown,
    DpadLeft,
    Count
};

/// @brief A gamepad analog axis. Sticks read [-1, 1] (Y up-positive), triggers read [0, 1].
enum class GamepadAxis : std::uint8_t { LeftX, LeftY, RightX, RightY, LeftTrigger, RightTrigger, Count };

/// @brief Modifier keys held during a key or mouse button event, e.g. if (event.mods.control).
struct Mods {
    bool shift = false;
    bool control = false;
    bool alt = false;
    bool super = false;  ///< Cmd on macOS, Windows key on Windows.
    bool capsLock = false;
    bool numLock = false;
};

}  // namespace rtype::engine::input
