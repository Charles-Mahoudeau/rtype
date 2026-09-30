/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <exception>
#include <iostream>
#include <variant>

#include "engine/event/Event.hpp"
#include "engine/input/Control.hpp"
#include "engine/input/Input.hpp"
#include "engine/input/InputAction.hpp"
#include "engine/input/Key.hpp"
#include "platform/Window.hpp"

using rtype::engine::input::GamepadAxis;
using rtype::engine::input::GamepadButton;
using rtype::engine::input::Key;
using rtype::engine::input::MouseButton;
using namespace rtype::engine::event;
using rtype::engine::input::ActionType;
using rtype::engine::input::Control;

int main() {
    try {
        /// @note Create a window with the specified width, height, and title
        rtype::platform::Window window(800, 600, "Vulkan Window Example");
        /// @note Input does not know the window: it only reads the events the window produces.
        rtype::engine::input::Input input;

        /// @note Declare the actions once. Each one can be driven by any number of devices.

        /// @note Vector2: WASD, arrows and the left stick all move the player.
        auto& move =
            input.addAction("move", ActionType::Vector2)
                .bindVector(Control::key(Key::W), Control::key(Key::S), Control::key(Key::A), Control::key(Key::D))
                .bindVector(Control::key(Key::Up), Control::key(Key::Down), Control::key(Key::Left),
                            Control::key(Key::Right))
                .bindVector(Control::gamepadAxis(GamepadAxis::LeftX), Control::gamepadAxis(GamepadAxis::LeftY));

        /// @note Button: space, left click or the A button.
        auto& fire = input.addAction("fire", ActionType::Button)
                         .bind(Control::key(Key::Space))
                         .bind(Control::mouseButton(MouseButton::Left))
                         .bind(Control::gamepadButton(GamepadButton::South));

        /// @note Axis: Q / E or the gamepad triggers.
        auto& throttle = input.addAction("throttle", ActionType::Axis)
                             .bindAxis(Control::key(Key::Q), Control::key(Key::E))
                             .bindAxis(Control::gamepadAxis(GamepadAxis::LeftTrigger),
                                       Control::gamepadAxis(GamepadAxis::RightTrigger));

        /// @note Vector2: mouse movement or the right stick.
        auto& look =
            input.addAction("look", ActionType::Vector2)
                .bindVector(Control::mouseDeltaX(0.1F), Control::mouseDeltaY(0.1F))
                .bindVector(Control::gamepadAxis(GamepadAxis::RightX), Control::gamepadAxis(GamepadAxis::RightY));

        input.addAction("lock", ActionType::Button).bind(Control::key(Key::L));
        input.addAction("unlock", ActionType::Button).bind(Control::key(Key::U));

        while (window.isOpen()) {
            /// @note Poll for window events and update the window state
            for (const auto& event : window.pollEvents()) {
                input.handleEvent(event);

                /// @note Using the raw event system
                if (const auto* key = std::get_if<KeyPressed>(&event); (key != nullptr) && key->key == Key::Escape) {
                    std::cout << "Escape pressed, closing window.\n";
                    window.close();
                } else if (const auto* resized = std::get_if<Resized>(&event)) {
                    std::cout << "Window resized to " << resized->width << "x" << resized->height << "\n";
                } else if (const auto* text = std::get_if<TextEntered>(&event)) {
                    std::cout << "Text entered: " << static_cast<char>(text->codepoint) << "\n";
                }
            }

            /// @note Using action input system, once every event of the frame was handled
            /// @{
            input.update();

            if (auto dir = move.readVector(); dir.x != 0.0F || dir.y != 0.0F) {
                std::cout << "Move: (" << dir.x << ", " << dir.y << ")\n";
            }
            if (fire.isPressed()) {
                std::cout << "Fire!\n";
            }
            if (const float value = throttle.readAxis(); value != 0.0F) {
                std::cout << "Throttle: " << value << "\n";
            }
            if (auto delta = look.readVector(); delta.x != 0.0F || delta.y != 0.0F) {
                std::cout << "Look: (" << delta.x << ", " << delta.y << ")\n";
            }

            /// @note Actions can also be fetched by name.
            if (input.getAction("lock").isPressed()) {
                window.setCursorLocked(true);
            } else if (input.getAction("unlock").isPressed()) {
                window.setCursorLocked(false);
            }
            /// @}
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
