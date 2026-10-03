/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Key
*/

#pragma once

#include <cstdint>

namespace rtype::engine::input {

/// @brief A physical keyboard key.
///
/// @details Keys are named after their position on a US QWERTY layout, not after the
/// character they type: Key::kW is the key right of Tab on every layout (Z on AZERTY).
/// This keeps bindings like WASD in the same place for everyone. Use the TextEntered
/// event to read typed characters.
enum class Key : std::uint8_t {
    kUnknown,  ///< A key the engine does not know (media keys, vendor keys...).

    kSpace,
    kApostrophe,
    kComma,
    kMinus,
    kPeriod,
    kSlash,
    kNum0,
    kNum1,
    kNum2,
    kNum3,
    kNum4,
    kNum5,
    kNum6,
    kNum7,
    kNum8,
    kNum9,
    kSemicolon,
    kEqual,
    kA,
    kB,
    kC,
    kD,
    kE,
    kF,
    kG,
    kH,
    kI,
    kJ,
    kK,
    kL,
    kM,
    kN,
    kO,
    kP,
    kQ,
    kR,
    kS,
    kT,
    kU,
    kV,
    kW,
    kX,
    kY,
    kZ,
    kLeftBracket,
    kBackslash,
    kRightBracket,
    kGrave,

    kEscape,
    kEnter,
    kTab,
    kBackspace,
    kInsert,
    kDelete,
    kRight,
    kLeft,
    kDown,
    kUp,
    kPageUp,
    kPageDown,
    kHome,
    kEnd,
    kCapsLock,
    kScrollLock,
    kNumLock,
    kPrintScreen,
    kPause,
    kF1,
    kF2,
    kF3,
    kF4,
    kF5,
    kF6,
    kF7,
    kF8,
    kF9,
    kF10,
    kF11,
    kF12,

    kKeypad0,
    kKeypad1,
    kKeypad2,
    kKeypad3,
    kKeypad4,
    kKeypad5,
    kKeypad6,
    kKeypad7,
    kKeypad8,
    kKeypad9,
    kKeypadDecimal,
    kKeypadDivide,
    kKeypadMultiply,
    kKeypadSubtract,
    kKeypadAdd,
    kKeypadEnter,
    kKeypadEqual,

    kLeftShift,
    kLeftControl,
    kLeftAlt,
    kLeftSuper,
    kRightShift,
    kRightControl,
    kRightAlt,
    kRightSuper,
    kMenu,

    kCount  ///< Number of keys, not a key. Useful to size lookup tables.
};

/// @brief A mouse button.
enum class MouseButton : std::uint8_t {
    kUnknown,  ///< A button the engine does not know.
    kLeft,
    kRight,
    kMiddle,
    kX1,  ///< First side button, usually "back".
    kX2,  ///< Second side button, usually "forward".
    kCount
};

/// @brief A gamepad button, named by position so that it means the same on every brand.
///
/// @details kSouth is A on Xbox, Cross on PlayStation and B on Nintendo.
enum class GamepadButton : std::uint8_t {
    kSouth,
    kEast,
    kWest,
    kNorth,
    kLeftBumper,
    kRightBumper,
    kBack,
    kStart,
    kGuide,
    kLeftThumb,   ///< Pressing the left stick.
    kRightThumb,  ///< Pressing the right stick.
    kDpadUp,
    kDpadRight,
    kDpadDown,
    kDpadLeft,
    kCount
};

/// @brief A gamepad analog axis. Sticks read [-1, 1] (Y up-positive), triggers read [0, 1].
enum class GamepadAxis : std::uint8_t { kLeftX, kLeftY, kRightX, kRightY, kLeftTrigger, kRightTrigger, kCount };

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
