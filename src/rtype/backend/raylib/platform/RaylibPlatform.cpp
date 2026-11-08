/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RaylibPlatform
*/

#include "RaylibPlatform.hpp"

#include <raylib.h>

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_uint2.hpp>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "InputMapping.hpp"
#include "rtype/contract/platform/WindowConfig.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"

namespace rtype::backend::raylib::platform {

namespace cp = contract::platform;

RaylibPlatform::~RaylibPlatform() {
    if (_initialized && IsWindowReady()) {
        CloseWindow();
    }
}

glm::uvec2 RaylibPlatform::getFramebufferSize() const {
    if (!_initialized) {
        return {0, 0};
    }
    return {static_cast<unsigned>(GetRenderWidth()), static_cast<unsigned>(GetRenderHeight())};
}

const std::string& RaylibPlatform::getTitle() const { return _title; }

void RaylibPlatform::setTitle(const std::string& title) {
    _title = title;
    if (_initialized) {
        SetWindowTitle(_title.c_str());
    }
}

void RaylibPlatform::setSize(glm::uvec2 size) {
    if (_initialized) {
        SetWindowSize(static_cast<int>(size.x), static_cast<int>(size.y));
    }
}

void RaylibPlatform::init(const cp::WindowConfig& config) {
    if (_initialized) {
        throw std::runtime_error("RaylibPlatform::init() called twice");
    }
    _title = config.title;
    SetTraceLogLevel(LOG_WARNING);
    unsigned int flags = FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT;
    if (config.resizable) {
        flags |= FLAG_WINDOW_RESIZABLE;
    }
    if (config.fullscreen) {
        flags |= FLAG_FULLSCREEN_MODE;
    }
    SetConfigFlags(flags);
    InitWindow(static_cast<int>(config.size.x), static_cast<int>(config.size.y), _title.c_str());
    if (!IsWindowReady()) {
        throw std::runtime_error("raylib could not open its window");
    }
    // The game decides what Escape does.
    SetExitKey(KEY_NULL);
    _initialized = true;
    _isOpen = true;
    _focused = IsWindowFocused();
}

std::vector<cp::Event> RaylibPlatform::pollEvents() {
    std::vector<cp::Event> events;
    if (!_initialized) {
        return events;
    }
    if (WindowShouldClose()) {
        _isOpen = false;
        events.emplace_back(cp::event::Closed{});
    }
    if (IsWindowResized()) {
        events.emplace_back(cp::event::Resized{.width = static_cast<std::uint16_t>(GetRenderWidth()),
                                               .height = static_cast<std::uint16_t>(GetRenderHeight())});
    }
    if (const bool focused = IsWindowFocused(); focused != _focused) {
        _focused = focused;
        events.emplace_back(cp::event::FocusChanged{.focused = focused});
    }

    const cp::input::Mods mods = currentMods();
    for (const KeyPair& pair : keys()) {
        if (IsKeyPressed(pair.raylib)) {
            events.emplace_back(cp::event::KeyPressed{.key = pair.key, .mods = mods, .repeat = false});
        } else if (IsKeyPressedRepeat(pair.raylib)) {
            events.emplace_back(cp::event::KeyPressed{.key = pair.key, .mods = mods, .repeat = true});
        }
        if (IsKeyReleased(pair.raylib)) {
            events.emplace_back(cp::event::KeyReleased{.key = pair.key, .mods = mods});
        }
    }
    for (int codepoint = GetCharPressed(); codepoint != 0; codepoint = GetCharPressed()) {
        events.emplace_back(cp::event::TextEntered{.codepoint = static_cast<char32_t>(codepoint)});
    }

    if (const Vector2 delta = GetMouseDelta(); delta.x != 0.0F || delta.y != 0.0F) {
        const Vector2 position = GetMousePosition();
        events.emplace_back(cp::event::MouseMoved{.position = {position.x, position.y}, .delta = {delta.x, delta.y}});
    }
    for (const MousePair& pair : mouseButtons()) {
        if (IsMouseButtonPressed(pair.raylib)) {
            events.emplace_back(cp::event::MouseButtonPressed{.button = pair.button, .mods = mods});
        }
        if (IsMouseButtonReleased(pair.raylib)) {
            events.emplace_back(cp::event::MouseButtonReleased{.button = pair.button, .mods = mods});
        }
    }
    if (const Vector2 wheel = GetMouseWheelMoveV(); wheel.x != 0.0F || wheel.y != 0.0F) {
        events.emplace_back(cp::event::MouseScrolled{.offset = {wheel.x, wheel.y}});
    }
    pollGamepad(events);
    return events;
}

void RaylibPlatform::pollGamepad(std::vector<cp::Event>& events) {
    const bool connected = IsGamepadAvailable(kGamepad);
    if (connected != _gamepad) {
        _gamepad = connected;
        if (connected) {
            events.emplace_back(cp::event::GamepadConnected{});
        } else {
            _axes.fill(0.0F);
            events.emplace_back(cp::event::GamepadDisconnected{});
        }
    }
    if (!connected) {
        return;
    }
    for (const PadPair& pair : gamepadButtons()) {
        if (IsGamepadButtonPressed(kGamepad, pair.raylib)) {
            events.emplace_back(cp::event::GamepadButtonPressed{.button = pair.button});
        }
        if (IsGamepadButtonReleased(kGamepad, pair.raylib)) {
            events.emplace_back(cp::event::GamepadButtonReleased{.button = pair.button});
        }
    }
    for (std::size_t index = 0; index < _axes.size(); ++index) {
        const auto axis = static_cast<cp::input::GamepadAxis>(index);
        const float value = readAxis(axis);
        if (value != _axes.at(index)) {
            _axes.at(index) = value;
            events.emplace_back(cp::event::GamepadAxisMoved{.axis = axis, .value = value});
        }
    }
}

bool RaylibPlatform::isOpen() const noexcept { return _isOpen; }

void RaylibPlatform::close() { _isOpen = false; }

void RaylibPlatform::setCursorLocked(bool locked) {
    if (!_initialized) {
        return;
    }
    if (locked) {
        DisableCursor();
    } else {
        EnableCursor();
    }
}

double RaylibPlatform::getTime() const { return GetTime(); }

void RaylibPlatform::setClipboardText(std::string_view text) {
    if (_initialized) {
        SetClipboardText(std::string(text).c_str());
    }
}

std::string RaylibPlatform::getClipboardText() const {
    if (!_initialized) {
        return {};
    }
    const char* text = GetClipboardText();
    return text != nullptr ? text : "";
}

}  // namespace rtype::backend::raylib::platform
