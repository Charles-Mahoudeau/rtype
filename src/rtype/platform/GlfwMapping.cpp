/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GlfwMapping
*/

#include "GlfwMapping.hpp"

#include <GLFW/glfw3.h>

#include <array>
#include <cstddef>

#include "engine/input/Key.hpp"

namespace rtype::platform::glfw {

using engine::input::GamepadAxis;
using engine::input::GamepadButton;
using engine::input::Key;
using engine::input::Mods;
using engine::input::MouseButton;

namespace {

struct KeyPair {
    Key key;
    int glfw;
};

/// @brief Single source of truth for keys, both lookup tables below are built from it.
constexpr auto kKeys = std::to_array<KeyPair>({
    {.key = Key::kSpace, .glfw = GLFW_KEY_SPACE},
    {.key = Key::kApostrophe, .glfw = GLFW_KEY_APOSTROPHE},
    {.key = Key::kComma, .glfw = GLFW_KEY_COMMA},
    {.key = Key::kMinus, .glfw = GLFW_KEY_MINUS},
    {.key = Key::kPeriod, .glfw = GLFW_KEY_PERIOD},
    {.key = Key::kSlash, .glfw = GLFW_KEY_SLASH},
    {.key = Key::kNum0, .glfw = GLFW_KEY_0},
    {.key = Key::kNum1, .glfw = GLFW_KEY_1},
    {.key = Key::kNum2, .glfw = GLFW_KEY_2},
    {.key = Key::kNum3, .glfw = GLFW_KEY_3},
    {.key = Key::kNum4, .glfw = GLFW_KEY_4},
    {.key = Key::kNum5, .glfw = GLFW_KEY_5},
    {.key = Key::kNum6, .glfw = GLFW_KEY_6},
    {.key = Key::kNum7, .glfw = GLFW_KEY_7},
    {.key = Key::kNum8, .glfw = GLFW_KEY_8},
    {.key = Key::kNum9, .glfw = GLFW_KEY_9},
    {.key = Key::kSemicolon, .glfw = GLFW_KEY_SEMICOLON},
    {.key = Key::kEqual, .glfw = GLFW_KEY_EQUAL},
    {.key = Key::kA, .glfw = GLFW_KEY_A},
    {.key = Key::kB, .glfw = GLFW_KEY_B},
    {.key = Key::kC, .glfw = GLFW_KEY_C},
    {.key = Key::kD, .glfw = GLFW_KEY_D},
    {.key = Key::kE, .glfw = GLFW_KEY_E},
    {.key = Key::kF, .glfw = GLFW_KEY_F},
    {.key = Key::kG, .glfw = GLFW_KEY_G},
    {.key = Key::kH, .glfw = GLFW_KEY_H},
    {.key = Key::kI, .glfw = GLFW_KEY_I},
    {.key = Key::kJ, .glfw = GLFW_KEY_J},
    {.key = Key::kK, .glfw = GLFW_KEY_K},
    {.key = Key::kL, .glfw = GLFW_KEY_L},
    {.key = Key::kM, .glfw = GLFW_KEY_M},
    {.key = Key::kN, .glfw = GLFW_KEY_N},
    {.key = Key::kO, .glfw = GLFW_KEY_O},
    {.key = Key::kP, .glfw = GLFW_KEY_P},
    {.key = Key::kQ, .glfw = GLFW_KEY_Q},
    {.key = Key::kR, .glfw = GLFW_KEY_R},
    {.key = Key::kS, .glfw = GLFW_KEY_S},
    {.key = Key::kT, .glfw = GLFW_KEY_T},
    {.key = Key::kU, .glfw = GLFW_KEY_U},
    {.key = Key::kV, .glfw = GLFW_KEY_V},
    {.key = Key::kW, .glfw = GLFW_KEY_W},
    {.key = Key::kX, .glfw = GLFW_KEY_X},
    {.key = Key::kY, .glfw = GLFW_KEY_Y},
    {.key = Key::kZ, .glfw = GLFW_KEY_Z},
    {.key = Key::kLeftBracket, .glfw = GLFW_KEY_LEFT_BRACKET},
    {.key = Key::kBackslash, .glfw = GLFW_KEY_BACKSLASH},
    {.key = Key::kRightBracket, .glfw = GLFW_KEY_RIGHT_BRACKET},
    {.key = Key::kGrave, .glfw = GLFW_KEY_GRAVE_ACCENT},
    {.key = Key::kEscape, .glfw = GLFW_KEY_ESCAPE},
    {.key = Key::kEnter, .glfw = GLFW_KEY_ENTER},
    {.key = Key::kTab, .glfw = GLFW_KEY_TAB},
    {.key = Key::kBackspace, .glfw = GLFW_KEY_BACKSPACE},
    {.key = Key::kInsert, .glfw = GLFW_KEY_INSERT},
    {.key = Key::kDelete, .glfw = GLFW_KEY_DELETE},
    {.key = Key::kRight, .glfw = GLFW_KEY_RIGHT},
    {.key = Key::kLeft, .glfw = GLFW_KEY_LEFT},
    {.key = Key::kDown, .glfw = GLFW_KEY_DOWN},
    {.key = Key::kUp, .glfw = GLFW_KEY_UP},
    {.key = Key::kPageUp, .glfw = GLFW_KEY_PAGE_UP},
    {.key = Key::kPageDown, .glfw = GLFW_KEY_PAGE_DOWN},
    {.key = Key::kHome, .glfw = GLFW_KEY_HOME},
    {.key = Key::kEnd, .glfw = GLFW_KEY_END},
    {.key = Key::kCapsLock, .glfw = GLFW_KEY_CAPS_LOCK},
    {.key = Key::kScrollLock, .glfw = GLFW_KEY_SCROLL_LOCK},
    {.key = Key::kNumLock, .glfw = GLFW_KEY_NUM_LOCK},
    {.key = Key::kPrintScreen, .glfw = GLFW_KEY_PRINT_SCREEN},
    {.key = Key::kPause, .glfw = GLFW_KEY_PAUSE},
    {.key = Key::kF1, .glfw = GLFW_KEY_F1},
    {.key = Key::kF2, .glfw = GLFW_KEY_F2},
    {.key = Key::kF3, .glfw = GLFW_KEY_F3},
    {.key = Key::kF4, .glfw = GLFW_KEY_F4},
    {.key = Key::kF5, .glfw = GLFW_KEY_F5},
    {.key = Key::kF6, .glfw = GLFW_KEY_F6},
    {.key = Key::kF7, .glfw = GLFW_KEY_F7},
    {.key = Key::kF8, .glfw = GLFW_KEY_F8},
    {.key = Key::kF9, .glfw = GLFW_KEY_F9},
    {.key = Key::kF10, .glfw = GLFW_KEY_F10},
    {.key = Key::kF11, .glfw = GLFW_KEY_F11},
    {.key = Key::kF12, .glfw = GLFW_KEY_F12},
    {.key = Key::kKeypad0, .glfw = GLFW_KEY_KP_0},
    {.key = Key::kKeypad1, .glfw = GLFW_KEY_KP_1},
    {.key = Key::kKeypad2, .glfw = GLFW_KEY_KP_2},
    {.key = Key::kKeypad3, .glfw = GLFW_KEY_KP_3},
    {.key = Key::kKeypad4, .glfw = GLFW_KEY_KP_4},
    {.key = Key::kKeypad5, .glfw = GLFW_KEY_KP_5},
    {.key = Key::kKeypad6, .glfw = GLFW_KEY_KP_6},
    {.key = Key::kKeypad7, .glfw = GLFW_KEY_KP_7},
    {.key = Key::kKeypad8, .glfw = GLFW_KEY_KP_8},
    {.key = Key::kKeypad9, .glfw = GLFW_KEY_KP_9},
    {.key = Key::kKeypadDecimal, .glfw = GLFW_KEY_KP_DECIMAL},
    {.key = Key::kKeypadDivide, .glfw = GLFW_KEY_KP_DIVIDE},
    {.key = Key::kKeypadMultiply, .glfw = GLFW_KEY_KP_MULTIPLY},
    {.key = Key::kKeypadSubtract, .glfw = GLFW_KEY_KP_SUBTRACT},
    {.key = Key::kKeypadAdd, .glfw = GLFW_KEY_KP_ADD},
    {.key = Key::kKeypadEnter, .glfw = GLFW_KEY_KP_ENTER},
    {.key = Key::kKeypadEqual, .glfw = GLFW_KEY_KP_EQUAL},
    {.key = Key::kLeftShift, .glfw = GLFW_KEY_LEFT_SHIFT},
    {.key = Key::kLeftControl, .glfw = GLFW_KEY_LEFT_CONTROL},
    {.key = Key::kLeftAlt, .glfw = GLFW_KEY_LEFT_ALT},
    {.key = Key::kLeftSuper, .glfw = GLFW_KEY_LEFT_SUPER},
    {.key = Key::kRightShift, .glfw = GLFW_KEY_RIGHT_SHIFT},
    {.key = Key::kRightControl, .glfw = GLFW_KEY_RIGHT_CONTROL},
    {.key = Key::kRightAlt, .glfw = GLFW_KEY_RIGHT_ALT},
    {.key = Key::kRightSuper, .glfw = GLFW_KEY_RIGHT_SUPER},
    {.key = Key::kMenu, .glfw = GLFW_KEY_MENU},
});

static_assert(kKeys.size() == static_cast<std::size_t>(Key::kCount) - 1,
              "Every Key except kUnknown needs exactly one GLFW mapping");

/// @brief Engine key -> GLFW key code, indexed by Key.
constexpr auto kToGlfwKey = [] {
    std::array<int, static_cast<std::size_t>(Key::kCount)> table{};
    table.fill(GLFW_KEY_UNKNOWN);
    for (const auto& [key, glfw] : kKeys) {
        table.at(static_cast<std::size_t>(key)) = glfw;
    }
    return table;
}();

/// @brief GLFW key code -> engine key, indexed by GLFW key code.
constexpr auto kFromGlfwKey = [] {
    std::array<Key, GLFW_KEY_LAST + 1> table{};
    table.fill(Key::kUnknown);
    for (const auto& [key, glfw] : kKeys) {
        table.at(static_cast<std::size_t>(glfw)) = key;
    }
    return table;
}();

}  // namespace

Key fromGlfwKey(int key) {
    if (key < 0 || key > GLFW_KEY_LAST) {
        return Key::kUnknown;
    }
    return kFromGlfwKey.at(static_cast<std::size_t>(key));
}

int toGlfwKey(Key key) {
    if (key >= Key::kCount) {
        return GLFW_KEY_UNKNOWN;
    }
    return kToGlfwKey.at(static_cast<std::size_t>(key));
}

MouseButton fromGlfwMouseButton(int button) noexcept {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return MouseButton::kLeft;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return MouseButton::kRight;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return MouseButton::kMiddle;
        case GLFW_MOUSE_BUTTON_4:
            return MouseButton::kX1;
        case GLFW_MOUSE_BUTTON_5:
            return MouseButton::kX2;
        default:
            return MouseButton::kUnknown;
    }
}

int toGlfwMouseButton(MouseButton button) noexcept {
    switch (button) {
        case MouseButton::kLeft:
            return GLFW_MOUSE_BUTTON_LEFT;
        case MouseButton::kRight:
            return GLFW_MOUSE_BUTTON_RIGHT;
        case MouseButton::kMiddle:
            return GLFW_MOUSE_BUTTON_MIDDLE;
        case MouseButton::kX1:
            return GLFW_MOUSE_BUTTON_4;
        case MouseButton::kX2:
            return GLFW_MOUSE_BUTTON_5;
        case MouseButton::kUnknown:
        case MouseButton::kCount:
            break;
    }
    return -1;
}

int toGlfwGamepadButton(GamepadButton button) noexcept {
    switch (button) {
        case GamepadButton::kSouth:
            return GLFW_GAMEPAD_BUTTON_A;
        case GamepadButton::kEast:
            return GLFW_GAMEPAD_BUTTON_B;
        case GamepadButton::kWest:
            return GLFW_GAMEPAD_BUTTON_X;
        case GamepadButton::kNorth:
            return GLFW_GAMEPAD_BUTTON_Y;
        case GamepadButton::kLeftBumper:
            return GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
        case GamepadButton::kRightBumper:
            return GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
        case GamepadButton::kBack:
            return GLFW_GAMEPAD_BUTTON_BACK;
        case GamepadButton::kStart:
            return GLFW_GAMEPAD_BUTTON_START;
        case GamepadButton::kGuide:
            return GLFW_GAMEPAD_BUTTON_GUIDE;
        case GamepadButton::kLeftThumb:
            return GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
        case GamepadButton::kRightThumb:
            return GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
        case GamepadButton::kDpadUp:
            return GLFW_GAMEPAD_BUTTON_DPAD_UP;
        case GamepadButton::kDpadRight:
            return GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
        case GamepadButton::kDpadDown:
            return GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
        case GamepadButton::kDpadLeft:
            return GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
        case GamepadButton::kCount:
            break;
    }
    return -1;
}

int toGlfwGamepadAxis(GamepadAxis axis) noexcept {
    switch (axis) {
        case GamepadAxis::kLeftX:
            return GLFW_GAMEPAD_AXIS_LEFT_X;
        case GamepadAxis::kLeftY:
            return GLFW_GAMEPAD_AXIS_LEFT_Y;
        case GamepadAxis::kRightX:
            return GLFW_GAMEPAD_AXIS_RIGHT_X;
        case GamepadAxis::kRightY:
            return GLFW_GAMEPAD_AXIS_RIGHT_Y;
        case GamepadAxis::kLeftTrigger:
            return GLFW_GAMEPAD_AXIS_LEFT_TRIGGER;
        case GamepadAxis::kRightTrigger:
            return GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;
        case GamepadAxis::kCount:
            break;
    }
    return -1;
}

float fromGlfwGamepadAxisValue(GamepadAxis axis, float value) noexcept {
    switch (axis) {
        case GamepadAxis::kLeftTrigger:
        case GamepadAxis::kRightTrigger:
            return (value + 1.0F) * 0.5F;
        case GamepadAxis::kLeftY:
        case GamepadAxis::kRightY:
            return -value;
        case GamepadAxis::kLeftX:
        case GamepadAxis::kRightX:
        case GamepadAxis::kCount:
            break;
    }
    return value;
}

Mods fromGlfwMods(int mods) noexcept {
    const auto has = [mods](int flag) { return (static_cast<unsigned>(mods) & static_cast<unsigned>(flag)) != 0U; };
    return {
        .shift = has(GLFW_MOD_SHIFT),
        .control = has(GLFW_MOD_CONTROL),
        .alt = has(GLFW_MOD_ALT),
        .super = has(GLFW_MOD_SUPER),
        .capsLock = has(GLFW_MOD_CAPS_LOCK),
        .numLock = has(GLFW_MOD_NUM_LOCK),
    };
}

}  // namespace rtype::platform::glfw
