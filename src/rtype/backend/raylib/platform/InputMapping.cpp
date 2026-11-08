/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InputMapping
*/

#include "InputMapping.hpp"

#include <raylib.h>

#include <array>
#include <span>

#include "rtype/contract/platform/input/Key.hpp"

namespace rtype::backend::raylib::platform {
namespace {

using contract::platform::input::GamepadAxis;
using contract::platform::input::GamepadButton;
using contract::platform::input::Key;
using contract::platform::input::MouseButton;

/// @brief Every key the engine knows, with its raylib name.
constexpr std::array kKeys{
    KeyPair{.raylib = KEY_SPACE, .key = Key::kSpace},
    KeyPair{.raylib = KEY_APOSTROPHE, .key = Key::kApostrophe},
    KeyPair{.raylib = KEY_COMMA, .key = Key::kComma},
    KeyPair{.raylib = KEY_MINUS, .key = Key::kMinus},
    KeyPair{.raylib = KEY_PERIOD, .key = Key::kPeriod},
    KeyPair{.raylib = KEY_SLASH, .key = Key::kSlash},
    KeyPair{.raylib = KEY_ZERO, .key = Key::kNum0},
    KeyPair{.raylib = KEY_ONE, .key = Key::kNum1},
    KeyPair{.raylib = KEY_TWO, .key = Key::kNum2},
    KeyPair{.raylib = KEY_THREE, .key = Key::kNum3},
    KeyPair{.raylib = KEY_FOUR, .key = Key::kNum4},
    KeyPair{.raylib = KEY_FIVE, .key = Key::kNum5},
    KeyPair{.raylib = KEY_SIX, .key = Key::kNum6},
    KeyPair{.raylib = KEY_SEVEN, .key = Key::kNum7},
    KeyPair{.raylib = KEY_EIGHT, .key = Key::kNum8},
    KeyPair{.raylib = KEY_NINE, .key = Key::kNum9},
    KeyPair{.raylib = KEY_SEMICOLON, .key = Key::kSemicolon},
    KeyPair{.raylib = KEY_EQUAL, .key = Key::kEqual},
    KeyPair{.raylib = KEY_A, .key = Key::kA},
    KeyPair{.raylib = KEY_B, .key = Key::kB},
    KeyPair{.raylib = KEY_C, .key = Key::kC},
    KeyPair{.raylib = KEY_D, .key = Key::kD},
    KeyPair{.raylib = KEY_E, .key = Key::kE},
    KeyPair{.raylib = KEY_F, .key = Key::kF},
    KeyPair{.raylib = KEY_G, .key = Key::kG},
    KeyPair{.raylib = KEY_H, .key = Key::kH},
    KeyPair{.raylib = KEY_I, .key = Key::kI},
    KeyPair{.raylib = KEY_J, .key = Key::kJ},
    KeyPair{.raylib = KEY_K, .key = Key::kK},
    KeyPair{.raylib = KEY_L, .key = Key::kL},
    KeyPair{.raylib = KEY_M, .key = Key::kM},
    KeyPair{.raylib = KEY_N, .key = Key::kN},
    KeyPair{.raylib = KEY_O, .key = Key::kO},
    KeyPair{.raylib = KEY_P, .key = Key::kP},
    KeyPair{.raylib = KEY_Q, .key = Key::kQ},
    KeyPair{.raylib = KEY_R, .key = Key::kR},
    KeyPair{.raylib = KEY_S, .key = Key::kS},
    KeyPair{.raylib = KEY_T, .key = Key::kT},
    KeyPair{.raylib = KEY_U, .key = Key::kU},
    KeyPair{.raylib = KEY_V, .key = Key::kV},
    KeyPair{.raylib = KEY_W, .key = Key::kW},
    KeyPair{.raylib = KEY_X, .key = Key::kX},
    KeyPair{.raylib = KEY_Y, .key = Key::kY},
    KeyPair{.raylib = KEY_Z, .key = Key::kZ},
    KeyPair{.raylib = KEY_LEFT_BRACKET, .key = Key::kLeftBracket},
    KeyPair{.raylib = KEY_BACKSLASH, .key = Key::kBackslash},
    KeyPair{.raylib = KEY_RIGHT_BRACKET, .key = Key::kRightBracket},
    KeyPair{.raylib = KEY_GRAVE, .key = Key::kGrave},
    KeyPair{.raylib = KEY_ESCAPE, .key = Key::kEscape},
    KeyPair{.raylib = KEY_ENTER, .key = Key::kEnter},
    KeyPair{.raylib = KEY_TAB, .key = Key::kTab},
    KeyPair{.raylib = KEY_BACKSPACE, .key = Key::kBackspace},
    KeyPair{.raylib = KEY_INSERT, .key = Key::kInsert},
    KeyPair{.raylib = KEY_DELETE, .key = Key::kDelete},
    KeyPair{.raylib = KEY_RIGHT, .key = Key::kRight},
    KeyPair{.raylib = KEY_LEFT, .key = Key::kLeft},
    KeyPair{.raylib = KEY_DOWN, .key = Key::kDown},
    KeyPair{.raylib = KEY_UP, .key = Key::kUp},
    KeyPair{.raylib = KEY_PAGE_UP, .key = Key::kPageUp},
    KeyPair{.raylib = KEY_PAGE_DOWN, .key = Key::kPageDown},
    KeyPair{.raylib = KEY_HOME, .key = Key::kHome},
    KeyPair{.raylib = KEY_END, .key = Key::kEnd},
    KeyPair{.raylib = KEY_CAPS_LOCK, .key = Key::kCapsLock},
    KeyPair{.raylib = KEY_SCROLL_LOCK, .key = Key::kScrollLock},
    KeyPair{.raylib = KEY_NUM_LOCK, .key = Key::kNumLock},
    KeyPair{.raylib = KEY_PRINT_SCREEN, .key = Key::kPrintScreen},
    KeyPair{.raylib = KEY_PAUSE, .key = Key::kPause},
    KeyPair{.raylib = KEY_F1, .key = Key::kF1},
    KeyPair{.raylib = KEY_F2, .key = Key::kF2},
    KeyPair{.raylib = KEY_F3, .key = Key::kF3},
    KeyPair{.raylib = KEY_F4, .key = Key::kF4},
    KeyPair{.raylib = KEY_F5, .key = Key::kF5},
    KeyPair{.raylib = KEY_F6, .key = Key::kF6},
    KeyPair{.raylib = KEY_F7, .key = Key::kF7},
    KeyPair{.raylib = KEY_F8, .key = Key::kF8},
    KeyPair{.raylib = KEY_F9, .key = Key::kF9},
    KeyPair{.raylib = KEY_F10, .key = Key::kF10},
    KeyPair{.raylib = KEY_F11, .key = Key::kF11},
    KeyPair{.raylib = KEY_F12, .key = Key::kF12},
    KeyPair{.raylib = KEY_KP_0, .key = Key::kKeypad0},
    KeyPair{.raylib = KEY_KP_1, .key = Key::kKeypad1},
    KeyPair{.raylib = KEY_KP_2, .key = Key::kKeypad2},
    KeyPair{.raylib = KEY_KP_3, .key = Key::kKeypad3},
    KeyPair{.raylib = KEY_KP_4, .key = Key::kKeypad4},
    KeyPair{.raylib = KEY_KP_5, .key = Key::kKeypad5},
    KeyPair{.raylib = KEY_KP_6, .key = Key::kKeypad6},
    KeyPair{.raylib = KEY_KP_7, .key = Key::kKeypad7},
    KeyPair{.raylib = KEY_KP_8, .key = Key::kKeypad8},
    KeyPair{.raylib = KEY_KP_9, .key = Key::kKeypad9},
    KeyPair{.raylib = KEY_KP_DECIMAL, .key = Key::kKeypadDecimal},
    KeyPair{.raylib = KEY_KP_DIVIDE, .key = Key::kKeypadDivide},
    KeyPair{.raylib = KEY_KP_MULTIPLY, .key = Key::kKeypadMultiply},
    KeyPair{.raylib = KEY_KP_SUBTRACT, .key = Key::kKeypadSubtract},
    KeyPair{.raylib = KEY_KP_ADD, .key = Key::kKeypadAdd},
    KeyPair{.raylib = KEY_KP_ENTER, .key = Key::kKeypadEnter},
    KeyPair{.raylib = KEY_KP_EQUAL, .key = Key::kKeypadEqual},
    KeyPair{.raylib = KEY_LEFT_SHIFT, .key = Key::kLeftShift},
    KeyPair{.raylib = KEY_LEFT_CONTROL, .key = Key::kLeftControl},
    KeyPair{.raylib = KEY_LEFT_ALT, .key = Key::kLeftAlt},
    KeyPair{.raylib = KEY_LEFT_SUPER, .key = Key::kLeftSuper},
    KeyPair{.raylib = KEY_RIGHT_SHIFT, .key = Key::kRightShift},
    KeyPair{.raylib = KEY_RIGHT_CONTROL, .key = Key::kRightControl},
    KeyPair{.raylib = KEY_RIGHT_ALT, .key = Key::kRightAlt},
    KeyPair{.raylib = KEY_RIGHT_SUPER, .key = Key::kRightSuper},
    KeyPair{.raylib = KEY_KB_MENU, .key = Key::kMenu},
};

constexpr std::array kMouseButtons{
    MousePair{.raylib = MOUSE_BUTTON_LEFT, .button = MouseButton::kLeft},
    MousePair{.raylib = MOUSE_BUTTON_RIGHT, .button = MouseButton::kRight},
    MousePair{.raylib = MOUSE_BUTTON_MIDDLE, .button = MouseButton::kMiddle},
    MousePair{.raylib = MOUSE_BUTTON_SIDE, .button = MouseButton::kX1},
    MousePair{.raylib = MOUSE_BUTTON_EXTRA, .button = MouseButton::kX2},
};

constexpr std::array kPadButtons{
    PadPair{.raylib = GAMEPAD_BUTTON_RIGHT_FACE_DOWN, .button = GamepadButton::kSouth},
    PadPair{.raylib = GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, .button = GamepadButton::kEast},
    PadPair{.raylib = GAMEPAD_BUTTON_RIGHT_FACE_LEFT, .button = GamepadButton::kWest},
    PadPair{.raylib = GAMEPAD_BUTTON_RIGHT_FACE_UP, .button = GamepadButton::kNorth},
    PadPair{.raylib = GAMEPAD_BUTTON_LEFT_TRIGGER_1, .button = GamepadButton::kLeftBumper},
    PadPair{.raylib = GAMEPAD_BUTTON_RIGHT_TRIGGER_1, .button = GamepadButton::kRightBumper},
    PadPair{.raylib = GAMEPAD_BUTTON_MIDDLE_LEFT, .button = GamepadButton::kBack},
    PadPair{.raylib = GAMEPAD_BUTTON_MIDDLE_RIGHT, .button = GamepadButton::kStart},
    PadPair{.raylib = GAMEPAD_BUTTON_MIDDLE, .button = GamepadButton::kGuide},
    PadPair{.raylib = GAMEPAD_BUTTON_LEFT_THUMB, .button = GamepadButton::kLeftThumb},
    PadPair{.raylib = GAMEPAD_BUTTON_RIGHT_THUMB, .button = GamepadButton::kRightThumb},
    PadPair{.raylib = GAMEPAD_BUTTON_LEFT_FACE_UP, .button = GamepadButton::kDpadUp},
    PadPair{.raylib = GAMEPAD_BUTTON_LEFT_FACE_RIGHT, .button = GamepadButton::kDpadRight},
    PadPair{.raylib = GAMEPAD_BUTTON_LEFT_FACE_DOWN, .button = GamepadButton::kDpadDown},
    PadPair{.raylib = GAMEPAD_BUTTON_LEFT_FACE_LEFT, .button = GamepadButton::kDpadLeft},
};

}  // namespace

std::span<const KeyPair> keys() { return kKeys; }

std::span<const MousePair> mouseButtons() { return kMouseButtons; }

std::span<const PadPair> gamepadButtons() { return kPadButtons; }

contract::platform::input::Mods currentMods() {
    return {.shift = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT),
            .control = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL),
            .alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT),
            .super = IsKeyDown(KEY_LEFT_SUPER) || IsKeyDown(KEY_RIGHT_SUPER),
            .capsLock = false,
            .numLock = false};
}

float readAxis(GamepadAxis axis) {
    switch (axis) {
        case GamepadAxis::kLeftX:
            return GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_X);
        case GamepadAxis::kLeftY:
            return -GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_Y);
        case GamepadAxis::kRightX:
            return GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_RIGHT_X);
        case GamepadAxis::kRightY:
            return -GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_RIGHT_Y);
        case GamepadAxis::kLeftTrigger:
            return (GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_LEFT_TRIGGER) + 1.0F) / 2.0F;
        case GamepadAxis::kRightTrigger:
            return (GetGamepadAxisMovement(kGamepad, GAMEPAD_AXIS_RIGHT_TRIGGER) + 1.0F) / 2.0F;
        default:
            return 0.0F;
    }
}

}  // namespace rtype::backend::raylib::platform
