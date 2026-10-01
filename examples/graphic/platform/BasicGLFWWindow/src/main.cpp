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
#include "platform/GlfwPlatform.hpp"

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
        rtype::platform::GlfwPlatform window;
        window.init({.size = {800, 600}, .title = "GLFW Window Example"});
        /// @note Input does not know the window: it only reads the events the window produces.
        rtype::engine::input::Input input;

        /// @note Declare the actions once. Each one can be driven by any number of devices.

        /// @note kVector2: WASD, arrows and the left stick all move the player.
        auto& move =
            input.addAction("move", ActionType::kVector2)
                .bindVector(Control::key(Key::kW), Control::key(Key::kS), Control::key(Key::kA), Control::key(Key::kD))
                .bindVector(Control::key(Key::kUp), Control::key(Key::kDown), Control::key(Key::kLeft),
                            Control::key(Key::kRight))
                .bindVector(Control::gamepadAxis(GamepadAxis::kLeftX), Control::gamepadAxis(GamepadAxis::kLeftY));

        /// @note kButton: space, left click or the A button.
        auto& fire = input.addAction("fire", ActionType::kButton)
                         .bind(Control::key(Key::kSpace))
                         .bind(Control::mouseButton(MouseButton::kLeft))
                         .bind(Control::gamepadButton(GamepadButton::kSouth));

        /// @note kAxis: Q / E or the gamepad triggers.
        auto& throttle = input.addAction("throttle", ActionType::kAxis)
                             .bindAxis(Control::key(Key::kQ), Control::key(Key::kE))
                             .bindAxis(Control::gamepadAxis(GamepadAxis::kLeftTrigger),
                                       Control::gamepadAxis(GamepadAxis::kRightTrigger));

        /// @note kVector2: mouse movement or the right stick.
        auto& look =
            input.addAction("look", ActionType::kVector2)
                .bindVector(Control::mouseDeltaX(0.1F), Control::mouseDeltaY(0.1F))
                .bindVector(Control::gamepadAxis(GamepadAxis::kRightX), Control::gamepadAxis(GamepadAxis::kRightY));

        input.addAction("lock", ActionType::kButton).bind(Control::key(Key::kL));
        input.addAction("unlock", ActionType::kButton).bind(Control::key(Key::kU));

        while (window.isOpen()) {
            /// @note Poll for window events and update the window state
            for (const auto& event : window.pollEvents()) {
                input.handleEvent(event);

                /// @note Using the raw event system
                if (const auto* key = std::get_if<KeyPressed>(&event); (key != nullptr) && key->key == Key::kEscape) {
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
