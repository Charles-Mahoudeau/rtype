/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ControlNames
*/

#include "plugins/input/ControlNames.hpp"

#include <optional>
#include <string_view>

#include "plugins/input/Control.hpp"
#include "rtype/contract/platform/input/KeyNames.hpp"

namespace rtype::plugins::input {
namespace {
constexpr std::string_view kMousePrefix = "Mouse";      ///< Starts the name of a mouse control.
constexpr std::string_view kGamepadPrefix = "Gamepad";  ///< Starts the name of a gamepad control.

/// @return The mouse control named @p name, without its "Mouse" prefix, or nothing.
[[nodiscard]] std::optional<Control> mouseControl(const std::string_view name) noexcept {
    if (name == "DeltaX") {
        return Control::mouseDeltaX();
    }
    if (name == "DeltaY") {
        return Control::mouseDeltaY();
    }
    if (name == "ScrollX") {
        return Control::mouseScrollX();
    }
    if (name == "ScrollY") {
        return Control::mouseScrollY();
    }
    if (const auto button = contract::platform::input::mouseButtonFromName(name)) {
        return Control::mouseButton(*button);
    }
    return std::nullopt;
}

/// @return The gamepad control named @p name, without its "Gamepad" prefix, or nothing.
[[nodiscard]] std::optional<Control> gamepadControl(const std::string_view name) noexcept {
    if (const auto button = contract::platform::input::gamepadButtonFromName(name)) {
        return Control::gamepadButton(*button);
    }
    if (const auto axis = contract::platform::input::gamepadAxisFromName(name)) {
        return Control::gamepadAxis(*axis);
    }
    return std::nullopt;
}
}  // namespace

std::optional<Control> controlFromName(const std::string_view name) noexcept {
    if (const auto key = contract::platform::input::keyFromName(name)) {
        return Control::key(*key);
    }
    if (name.starts_with(kMousePrefix)) {
        return mouseControl(name.substr(kMousePrefix.size()));
    }
    if (name.starts_with(kGamepadPrefix)) {
        return gamepadControl(name.substr(kGamepadPrefix.size()));
    }
    return std::nullopt;
}

}  // namespace rtype::plugins::input
