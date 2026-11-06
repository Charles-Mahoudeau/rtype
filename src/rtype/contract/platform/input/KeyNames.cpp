/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** KeyNames
*/

#include "rtype/contract/platform/input/KeyNames.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <utility>

#include "rtype/contract/platform/input/Key.hpp"

namespace rtype::contract::platform::input {
namespace {
/// @brief Every Key but the unknown one, by name.
constexpr auto kKeys = std::to_array<std::pair<std::string_view, Key>>({
    {"Space", Key::kSpace},
    {"Apostrophe", Key::kApostrophe},
    {"Comma", Key::kComma},
    {"Minus", Key::kMinus},
    {"Period", Key::kPeriod},
    {"Slash", Key::kSlash},
    {"Num0", Key::kNum0},
    {"Num1", Key::kNum1},
    {"Num2", Key::kNum2},
    {"Num3", Key::kNum3},
    {"Num4", Key::kNum4},
    {"Num5", Key::kNum5},
    {"Num6", Key::kNum6},
    {"Num7", Key::kNum7},
    {"Num8", Key::kNum8},
    {"Num9", Key::kNum9},
    {"Semicolon", Key::kSemicolon},
    {"Equal", Key::kEqual},
    {"A", Key::kA},
    {"B", Key::kB},
    {"C", Key::kC},
    {"D", Key::kD},
    {"E", Key::kE},
    {"F", Key::kF},
    {"G", Key::kG},
    {"H", Key::kH},
    {"I", Key::kI},
    {"J", Key::kJ},
    {"K", Key::kK},
    {"L", Key::kL},
    {"M", Key::kM},
    {"N", Key::kN},
    {"O", Key::kO},
    {"P", Key::kP},
    {"Q", Key::kQ},
    {"R", Key::kR},
    {"S", Key::kS},
    {"T", Key::kT},
    {"U", Key::kU},
    {"V", Key::kV},
    {"W", Key::kW},
    {"X", Key::kX},
    {"Y", Key::kY},
    {"Z", Key::kZ},
    {"LeftBracket", Key::kLeftBracket},
    {"Backslash", Key::kBackslash},
    {"RightBracket", Key::kRightBracket},
    {"Grave", Key::kGrave},
    {"Escape", Key::kEscape},
    {"Enter", Key::kEnter},
    {"Tab", Key::kTab},
    {"Backspace", Key::kBackspace},
    {"Insert", Key::kInsert},
    {"Delete", Key::kDelete},
    {"Right", Key::kRight},
    {"Left", Key::kLeft},
    {"Down", Key::kDown},
    {"Up", Key::kUp},
    {"PageUp", Key::kPageUp},
    {"PageDown", Key::kPageDown},
    {"Home", Key::kHome},
    {"End", Key::kEnd},
    {"CapsLock", Key::kCapsLock},
    {"ScrollLock", Key::kScrollLock},
    {"NumLock", Key::kNumLock},
    {"PrintScreen", Key::kPrintScreen},
    {"Pause", Key::kPause},
    {"F1", Key::kF1},
    {"F2", Key::kF2},
    {"F3", Key::kF3},
    {"F4", Key::kF4},
    {"F5", Key::kF5},
    {"F6", Key::kF6},
    {"F7", Key::kF7},
    {"F8", Key::kF8},
    {"F9", Key::kF9},
    {"F10", Key::kF10},
    {"F11", Key::kF11},
    {"F12", Key::kF12},
    {"Keypad0", Key::kKeypad0},
    {"Keypad1", Key::kKeypad1},
    {"Keypad2", Key::kKeypad2},
    {"Keypad3", Key::kKeypad3},
    {"Keypad4", Key::kKeypad4},
    {"Keypad5", Key::kKeypad5},
    {"Keypad6", Key::kKeypad6},
    {"Keypad7", Key::kKeypad7},
    {"Keypad8", Key::kKeypad8},
    {"Keypad9", Key::kKeypad9},
    {"KeypadDecimal", Key::kKeypadDecimal},
    {"KeypadDivide", Key::kKeypadDivide},
    {"KeypadMultiply", Key::kKeypadMultiply},
    {"KeypadSubtract", Key::kKeypadSubtract},
    {"KeypadAdd", Key::kKeypadAdd},
    {"KeypadEnter", Key::kKeypadEnter},
    {"KeypadEqual", Key::kKeypadEqual},
    {"LeftShift", Key::kLeftShift},
    {"LeftControl", Key::kLeftControl},
    {"LeftAlt", Key::kLeftAlt},
    {"LeftSuper", Key::kLeftSuper},
    {"RightShift", Key::kRightShift},
    {"RightControl", Key::kRightControl},
    {"RightAlt", Key::kRightAlt},
    {"RightSuper", Key::kRightSuper},
    {"Menu", Key::kMenu},
});

/// @brief Every MouseButton but the unknown one, by name.
constexpr auto kMouseButtons = std::to_array<std::pair<std::string_view, MouseButton>>({
    {"Left", MouseButton::kLeft},
    {"Right", MouseButton::kRight},
    {"Middle", MouseButton::kMiddle},
    {"X1", MouseButton::kX1},
    {"X2", MouseButton::kX2},
});

/// @brief Every GamepadButton but the unknown one, by name.
constexpr auto kGamepadButtons = std::to_array<std::pair<std::string_view, GamepadButton>>({
    {"South", GamepadButton::kSouth},
    {"East", GamepadButton::kEast},
    {"West", GamepadButton::kWest},
    {"North", GamepadButton::kNorth},
    {"LeftBumper", GamepadButton::kLeftBumper},
    {"RightBumper", GamepadButton::kRightBumper},
    {"Back", GamepadButton::kBack},
    {"Start", GamepadButton::kStart},
    {"Guide", GamepadButton::kGuide},
    {"LeftThumb", GamepadButton::kLeftThumb},
    {"RightThumb", GamepadButton::kRightThumb},
    {"DpadUp", GamepadButton::kDpadUp},
    {"DpadRight", GamepadButton::kDpadRight},
    {"DpadDown", GamepadButton::kDpadDown},
    {"DpadLeft", GamepadButton::kDpadLeft},
});

/// @brief Every GamepadAxis but the unknown one, by name.
constexpr auto kGamepadAxes = std::to_array<std::pair<std::string_view, GamepadAxis>>({
    {"LeftX", GamepadAxis::kLeftX},
    {"LeftY", GamepadAxis::kLeftY},
    {"RightX", GamepadAxis::kRightX},
    {"RightY", GamepadAxis::kRightY},
    {"LeftTrigger", GamepadAxis::kLeftTrigger},
    {"RightTrigger", GamepadAxis::kRightTrigger},
});

/// @return The value named @p name in @p names, or nothing.
template <typename Enum, std::size_t Count>
[[nodiscard]] std::optional<Enum> findByName(const std::array<std::pair<std::string_view, Enum>, Count>& names,
                                             const std::string_view name) noexcept {
    const auto found = std::ranges::find(names, name, &std::pair<std::string_view, Enum>::first);
    if (found == names.end()) {
        return std::nullopt;
    }
    return found->second;
}
}  // namespace

std::optional<Key> keyFromName(const std::string_view name) noexcept { return findByName(kKeys, name); }

std::optional<MouseButton> mouseButtonFromName(const std::string_view name) noexcept {
    return findByName(kMouseButtons, name);
}

std::optional<GamepadButton> gamepadButtonFromName(const std::string_view name) noexcept {
    return findByName(kGamepadButtons, name);
}

std::optional<GamepadAxis> gamepadAxisFromName(const std::string_view name) noexcept {
    return findByName(kGamepadAxes, name);
}

}  // namespace rtype::contract::platform::input
