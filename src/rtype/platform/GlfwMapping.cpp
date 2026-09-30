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
constexpr auto KEYS = std::to_array<KeyPair>({
    {.key = Key::Space, .glfw = GLFW_KEY_SPACE},
    {.key = Key::Apostrophe, .glfw = GLFW_KEY_APOSTROPHE},
    {.key = Key::Comma, .glfw = GLFW_KEY_COMMA},
    {.key = Key::Minus, .glfw = GLFW_KEY_MINUS},
    {.key = Key::Period, .glfw = GLFW_KEY_PERIOD},
    {.key = Key::Slash, .glfw = GLFW_KEY_SLASH},
    {.key = Key::Num0, .glfw = GLFW_KEY_0},
    {.key = Key::Num1, .glfw = GLFW_KEY_1},
    {.key = Key::Num2, .glfw = GLFW_KEY_2},
    {.key = Key::Num3, .glfw = GLFW_KEY_3},
    {.key = Key::Num4, .glfw = GLFW_KEY_4},
    {.key = Key::Num5, .glfw = GLFW_KEY_5},
    {.key = Key::Num6, .glfw = GLFW_KEY_6},
    {.key = Key::Num7, .glfw = GLFW_KEY_7},
    {.key = Key::Num8, .glfw = GLFW_KEY_8},
    {.key = Key::Num9, .glfw = GLFW_KEY_9},
    {.key = Key::Semicolon, .glfw = GLFW_KEY_SEMICOLON},
    {.key = Key::Equal, .glfw = GLFW_KEY_EQUAL},
    {.key = Key::A, .glfw = GLFW_KEY_A},
    {.key = Key::B, .glfw = GLFW_KEY_B},
    {.key = Key::C, .glfw = GLFW_KEY_C},
    {.key = Key::D, .glfw = GLFW_KEY_D},
    {.key = Key::E, .glfw = GLFW_KEY_E},
    {.key = Key::F, .glfw = GLFW_KEY_F},
    {.key = Key::G, .glfw = GLFW_KEY_G},
    {.key = Key::H, .glfw = GLFW_KEY_H},
    {.key = Key::I, .glfw = GLFW_KEY_I},
    {.key = Key::J, .glfw = GLFW_KEY_J},
    {.key = Key::K, .glfw = GLFW_KEY_K},
    {.key = Key::L, .glfw = GLFW_KEY_L},
    {.key = Key::M, .glfw = GLFW_KEY_M},
    {.key = Key::N, .glfw = GLFW_KEY_N},
    {.key = Key::O, .glfw = GLFW_KEY_O},
    {.key = Key::P, .glfw = GLFW_KEY_P},
    {.key = Key::Q, .glfw = GLFW_KEY_Q},
    {.key = Key::R, .glfw = GLFW_KEY_R},
    {.key = Key::S, .glfw = GLFW_KEY_S},
    {.key = Key::T, .glfw = GLFW_KEY_T},
    {.key = Key::U, .glfw = GLFW_KEY_U},
    {.key = Key::V, .glfw = GLFW_KEY_V},
    {.key = Key::W, .glfw = GLFW_KEY_W},
    {.key = Key::X, .glfw = GLFW_KEY_X},
    {.key = Key::Y, .glfw = GLFW_KEY_Y},
    {.key = Key::Z, .glfw = GLFW_KEY_Z},
    {.key = Key::LeftBracket, .glfw = GLFW_KEY_LEFT_BRACKET},
    {.key = Key::Backslash, .glfw = GLFW_KEY_BACKSLASH},
    {.key = Key::RightBracket, .glfw = GLFW_KEY_RIGHT_BRACKET},
    {.key = Key::Grave, .glfw = GLFW_KEY_GRAVE_ACCENT},
    {.key = Key::Escape, .glfw = GLFW_KEY_ESCAPE},
    {.key = Key::Enter, .glfw = GLFW_KEY_ENTER},
    {.key = Key::Tab, .glfw = GLFW_KEY_TAB},
    {.key = Key::Backspace, .glfw = GLFW_KEY_BACKSPACE},
    {.key = Key::Insert, .glfw = GLFW_KEY_INSERT},
    {.key = Key::Delete, .glfw = GLFW_KEY_DELETE},
    {.key = Key::Right, .glfw = GLFW_KEY_RIGHT},
    {.key = Key::Left, .glfw = GLFW_KEY_LEFT},
    {.key = Key::Down, .glfw = GLFW_KEY_DOWN},
    {.key = Key::Up, .glfw = GLFW_KEY_UP},
    {.key = Key::PageUp, .glfw = GLFW_KEY_PAGE_UP},
    {.key = Key::PageDown, .glfw = GLFW_KEY_PAGE_DOWN},
    {.key = Key::Home, .glfw = GLFW_KEY_HOME},
    {.key = Key::End, .glfw = GLFW_KEY_END},
    {.key = Key::CapsLock, .glfw = GLFW_KEY_CAPS_LOCK},
    {.key = Key::ScrollLock, .glfw = GLFW_KEY_SCROLL_LOCK},
    {.key = Key::NumLock, .glfw = GLFW_KEY_NUM_LOCK},
    {.key = Key::PrintScreen, .glfw = GLFW_KEY_PRINT_SCREEN},
    {.key = Key::Pause, .glfw = GLFW_KEY_PAUSE},
    {.key = Key::F1, .glfw = GLFW_KEY_F1},
    {.key = Key::F2, .glfw = GLFW_KEY_F2},
    {.key = Key::F3, .glfw = GLFW_KEY_F3},
    {.key = Key::F4, .glfw = GLFW_KEY_F4},
    {.key = Key::F5, .glfw = GLFW_KEY_F5},
    {.key = Key::F6, .glfw = GLFW_KEY_F6},
    {.key = Key::F7, .glfw = GLFW_KEY_F7},
    {.key = Key::F8, .glfw = GLFW_KEY_F8},
    {.key = Key::F9, .glfw = GLFW_KEY_F9},
    {.key = Key::F10, .glfw = GLFW_KEY_F10},
    {.key = Key::F11, .glfw = GLFW_KEY_F11},
    {.key = Key::F12, .glfw = GLFW_KEY_F12},
    {.key = Key::Keypad0, .glfw = GLFW_KEY_KP_0},
    {.key = Key::Keypad1, .glfw = GLFW_KEY_KP_1},
    {.key = Key::Keypad2, .glfw = GLFW_KEY_KP_2},
    {.key = Key::Keypad3, .glfw = GLFW_KEY_KP_3},
    {.key = Key::Keypad4, .glfw = GLFW_KEY_KP_4},
    {.key = Key::Keypad5, .glfw = GLFW_KEY_KP_5},
    {.key = Key::Keypad6, .glfw = GLFW_KEY_KP_6},
    {.key = Key::Keypad7, .glfw = GLFW_KEY_KP_7},
    {.key = Key::Keypad8, .glfw = GLFW_KEY_KP_8},
    {.key = Key::Keypad9, .glfw = GLFW_KEY_KP_9},
    {.key = Key::KeypadDecimal, .glfw = GLFW_KEY_KP_DECIMAL},
    {.key = Key::KeypadDivide, .glfw = GLFW_KEY_KP_DIVIDE},
    {.key = Key::KeypadMultiply, .glfw = GLFW_KEY_KP_MULTIPLY},
    {.key = Key::KeypadSubtract, .glfw = GLFW_KEY_KP_SUBTRACT},
    {.key = Key::KeypadAdd, .glfw = GLFW_KEY_KP_ADD},
    {.key = Key::KeypadEnter, .glfw = GLFW_KEY_KP_ENTER},
    {.key = Key::KeypadEqual, .glfw = GLFW_KEY_KP_EQUAL},
    {.key = Key::LeftShift, .glfw = GLFW_KEY_LEFT_SHIFT},
    {.key = Key::LeftControl, .glfw = GLFW_KEY_LEFT_CONTROL},
    {.key = Key::LeftAlt, .glfw = GLFW_KEY_LEFT_ALT},
    {.key = Key::LeftSuper, .glfw = GLFW_KEY_LEFT_SUPER},
    {.key = Key::RightShift, .glfw = GLFW_KEY_RIGHT_SHIFT},
    {.key = Key::RightControl, .glfw = GLFW_KEY_RIGHT_CONTROL},
    {.key = Key::RightAlt, .glfw = GLFW_KEY_RIGHT_ALT},
    {.key = Key::RightSuper, .glfw = GLFW_KEY_RIGHT_SUPER},
    {.key = Key::Menu, .glfw = GLFW_KEY_MENU},
});

static_assert(KEYS.size() == static_cast<std::size_t>(Key::Count) - 1,
              "Every Key except Unknown needs exactly one GLFW mapping");

/// @brief Engine key -> GLFW key code, indexed by Key.
constexpr auto TO_GLFW_KEY = [] {
    std::array<int, static_cast<std::size_t>(Key::Count)> table{};
    table.fill(GLFW_KEY_UNKNOWN);
    for (const auto& [key, glfw] : KEYS) {
        table.at(static_cast<std::size_t>(key)) = glfw;
    }
    return table;
}();

/// @brief GLFW key code -> engine key, indexed by GLFW key code.
constexpr auto FROM_GLFW_KEY = [] {
    std::array<Key, GLFW_KEY_LAST + 1> table{};
    table.fill(Key::Unknown);
    for (const auto& [key, glfw] : KEYS) {
        table.at(static_cast<std::size_t>(glfw)) = key;
    }
    return table;
}();

}  // namespace

Key fromGlfwKey(int key) {
    if (key < 0 || key > GLFW_KEY_LAST) {
        return Key::Unknown;
    }
    return FROM_GLFW_KEY.at(static_cast<std::size_t>(key));
}

int toGlfwKey(Key key) {
    if (key >= Key::Count) {
        return GLFW_KEY_UNKNOWN;
    }
    return TO_GLFW_KEY.at(static_cast<std::size_t>(key));
}

MouseButton fromGlfwMouseButton(int button) noexcept {
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            return MouseButton::Left;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return MouseButton::Right;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return MouseButton::Middle;
        case GLFW_MOUSE_BUTTON_4:
            return MouseButton::X1;
        case GLFW_MOUSE_BUTTON_5:
            return MouseButton::X2;
        default:
            return MouseButton::Unknown;
    }
}

int toGlfwMouseButton(MouseButton button) noexcept {
    switch (button) {
        case MouseButton::Left:
            return GLFW_MOUSE_BUTTON_LEFT;
        case MouseButton::Right:
            return GLFW_MOUSE_BUTTON_RIGHT;
        case MouseButton::Middle:
            return GLFW_MOUSE_BUTTON_MIDDLE;
        case MouseButton::X1:
            return GLFW_MOUSE_BUTTON_4;
        case MouseButton::X2:
            return GLFW_MOUSE_BUTTON_5;
        case MouseButton::Unknown:
        case MouseButton::Count:
            break;
    }
    return -1;
}

int toGlfwGamepadButton(GamepadButton button) noexcept {
    switch (button) {
        case GamepadButton::South:
            return GLFW_GAMEPAD_BUTTON_A;
        case GamepadButton::East:
            return GLFW_GAMEPAD_BUTTON_B;
        case GamepadButton::West:
            return GLFW_GAMEPAD_BUTTON_X;
        case GamepadButton::North:
            return GLFW_GAMEPAD_BUTTON_Y;
        case GamepadButton::LeftBumper:
            return GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
        case GamepadButton::RightBumper:
            return GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
        case GamepadButton::Back:
            return GLFW_GAMEPAD_BUTTON_BACK;
        case GamepadButton::Start:
            return GLFW_GAMEPAD_BUTTON_START;
        case GamepadButton::Guide:
            return GLFW_GAMEPAD_BUTTON_GUIDE;
        case GamepadButton::LeftThumb:
            return GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
        case GamepadButton::RightThumb:
            return GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
        case GamepadButton::DpadUp:
            return GLFW_GAMEPAD_BUTTON_DPAD_UP;
        case GamepadButton::DpadRight:
            return GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
        case GamepadButton::DpadDown:
            return GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
        case GamepadButton::DpadLeft:
            return GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
        case GamepadButton::Count:
            break;
    }
    return -1;
}

int toGlfwGamepadAxis(GamepadAxis axis) noexcept {
    switch (axis) {
        case GamepadAxis::LeftX:
            return GLFW_GAMEPAD_AXIS_LEFT_X;
        case GamepadAxis::LeftY:
            return GLFW_GAMEPAD_AXIS_LEFT_Y;
        case GamepadAxis::RightX:
            return GLFW_GAMEPAD_AXIS_RIGHT_X;
        case GamepadAxis::RightY:
            return GLFW_GAMEPAD_AXIS_RIGHT_Y;
        case GamepadAxis::LeftTrigger:
            return GLFW_GAMEPAD_AXIS_LEFT_TRIGGER;
        case GamepadAxis::RightTrigger:
            return GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER;
        case GamepadAxis::Count:
            break;
    }
    return -1;
}

float fromGlfwGamepadAxisValue(GamepadAxis axis, float value) noexcept {
    switch (axis) {
        case GamepadAxis::LeftTrigger:
        case GamepadAxis::RightTrigger:
            return (value + 1.0F) * 0.5F;
        case GamepadAxis::LeftY:
        case GamepadAxis::RightY:
            return -value;
        case GamepadAxis::LeftX:
        case GamepadAxis::RightX:
        case GamepadAxis::Count:
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
