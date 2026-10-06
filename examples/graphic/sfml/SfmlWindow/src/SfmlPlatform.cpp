/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SfmlPlatform
*/

#include "SfmlPlatform.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "engine/event/Event.hpp"
#include "engine/input/Key.hpp"
#include "engine/platform/WindowConfig.hpp"

using namespace rtype::engine;

namespace {

/// @return The engine key at the same physical position. Only the common keys: the others read as kUnknown.
input::Key toKey(sf::Keyboard::Scancode scancode) {
    using Scan = sf::Keyboard::Scancode;
    const auto code = static_cast<int>(scancode);
    // Both enums list letters and digits in a row, so ranges map by offset.
    if (code >= static_cast<int>(Scan::A) && code <= static_cast<int>(Scan::Z)) {
        return static_cast<input::Key>(static_cast<int>(input::Key::kA) + (code - static_cast<int>(Scan::A)));
    }
    if (code >= static_cast<int>(Scan::Num1) && code <= static_cast<int>(Scan::Num9)) {
        return static_cast<input::Key>(static_cast<int>(input::Key::kNum1) + (code - static_cast<int>(Scan::Num1)));
    }
    switch (scancode) {
        case Scan::Num0:
            return input::Key::kNum0;
        case Scan::Space:
            return input::Key::kSpace;
        case Scan::Escape:
            return input::Key::kEscape;
        case Scan::Enter:
            return input::Key::kEnter;
        case Scan::Tab:
            return input::Key::kTab;
        case Scan::Backspace:
            return input::Key::kBackspace;
        case Scan::Right:
            return input::Key::kRight;
        case Scan::Left:
            return input::Key::kLeft;
        case Scan::Down:
            return input::Key::kDown;
        case Scan::Up:
            return input::Key::kUp;
        default:
            return input::Key::kUnknown;
    }
}

input::MouseButton toMouseButton(sf::Mouse::Button button) {
    switch (button) {
        case sf::Mouse::Button::Left:
            return input::MouseButton::kLeft;
        case sf::Mouse::Button::Right:
            return input::MouseButton::kRight;
        case sf::Mouse::Button::Middle:
            return input::MouseButton::kMiddle;
        case sf::Mouse::Button::Extra1:
            return input::MouseButton::kX1;
        case sf::Mouse::Button::Extra2:
            return input::MouseButton::kX2;
        default:
            return input::MouseButton::kUnknown;
    }
}

/// @return The modifiers SFML reports with key events. SFML has no lock keys state: capsLock and numLock stay false.
template <typename KeyEvent>
input::Mods toMods(const KeyEvent& event) {
    return {.shift = event.shift, .control = event.control, .alt = event.alt, .super = event.system};
}

}  // namespace

namespace example {

glm::uvec2 SfmlPlatform::getFramebufferSize() const {
    const sf::Vector2u size = _window->getSize();
    return {size.x, size.y};
}

const std::string& SfmlPlatform::getTitle() const { return _title; }

void SfmlPlatform::setTitle(const std::string& title) {
    _title = title;
    _window->setTitle(_title);
}

void SfmlPlatform::setSize(glm::uvec2 size) { _window->setSize({size.x, size.y}); }

void SfmlPlatform::init(const platform::WindowConfig& config) {
    if (_window != nullptr) {
        throw std::runtime_error("SfmlPlatform::init() called twice");
    }
    _title = config.title;
    std::uint32_t style = sf::Style::Titlebar | sf::Style::Close;
    if (config.resizable) {
        style |= sf::Style::Resize;
    }
    const sf::VideoMode mode =
        config.fullscreen ? sf::VideoMode::getDesktopMode() : sf::VideoMode({config.size.x, config.size.y});
    _window = std::make_unique<sf::RenderWindow>(mode, _title, style,
                                                 config.fullscreen ? sf::State::Fullscreen : sf::State::Windowed);
    _window->setVerticalSyncEnabled(true);
    _isOpen = true;
    _clock.restart();
}

std::vector<Event> SfmlPlatform::pollEvents() {
    std::vector<Event> events;
    while (const std::optional sfEvent = _window->pollEvent()) {
        if (sfEvent->is<sf::Event::Closed>()) {
            _isOpen = false;
            events.emplace_back(event::Closed{});
        } else if (const auto* resized = sfEvent->getIf<sf::Event::Resized>()) {
            events.emplace_back(event::Resized{.width = static_cast<std::uint16_t>(resized->size.x),
                                               .height = static_cast<std::uint16_t>(resized->size.y)});
        } else if (sfEvent->is<sf::Event::FocusGained>() || sfEvent->is<sf::Event::FocusLost>()) {
            events.emplace_back(event::FocusChanged{.focused = sfEvent->is<sf::Event::FocusGained>()});
        } else if (const auto* pressed = sfEvent->getIf<sf::Event::KeyPressed>()) {
            // SFML repeats KeyPressed while a key is held, without telling it apart: repeat stays false.
            events.emplace_back(event::KeyPressed{.key = toKey(pressed->scancode), .mods = toMods(*pressed)});
        } else if (const auto* released = sfEvent->getIf<sf::Event::KeyReleased>()) {
            events.emplace_back(event::KeyReleased{.key = toKey(released->scancode), .mods = toMods(*released)});
        } else if (const auto* moved = sfEvent->getIf<sf::Event::MouseMoved>()) {
            const glm::vec2 position{static_cast<float>(moved->position.x), static_cast<float>(moved->position.y)};
            const glm::vec2 delta = _hasCursorPosition ? position - _cursorPosition : glm::vec2(0.0F);
            _cursorPosition = position;
            _hasCursorPosition = true;
            events.emplace_back(event::MouseMoved{.position = position, .delta = delta});
        } else if (const auto* down = sfEvent->getIf<sf::Event::MouseButtonPressed>()) {
            events.emplace_back(event::MouseButtonPressed{.button = toMouseButton(down->button)});
        } else if (const auto* up = sfEvent->getIf<sf::Event::MouseButtonReleased>()) {
            events.emplace_back(event::MouseButtonReleased{.button = toMouseButton(up->button)});
        } else if (const auto* wheel = sfEvent->getIf<sf::Event::MouseWheelScrolled>()) {
            const bool vertical = wheel->wheel == sf::Mouse::Wheel::Vertical;
            events.emplace_back(event::MouseScrolled{.offset = vertical ? glm::vec2(0.0F, wheel->delta)
                                                                        : glm::vec2(wheel->delta, 0.0F)});
        }
    }
    return events;
}

bool SfmlPlatform::isOpen() const noexcept { return _isOpen; }

void SfmlPlatform::close() { _isOpen = false; }

void SfmlPlatform::setCursorLocked(bool locked) {
    _window->setMouseCursorGrabbed(locked);
    _window->setMouseCursorVisible(!locked);
}

double SfmlPlatform::getTime() const { return _clock.getElapsedTime().asSeconds(); }

sf::RenderWindow& SfmlPlatform::getWindow() {
    if (_window == nullptr) {
        throw std::runtime_error("SfmlPlatform::getWindow() called before init()");
    }
    return *_window;
}

}  // namespace example
