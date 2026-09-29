/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <GLFW/glfw3.h>

#include <exception>
#include <iostream>
#include <variant>

#include "platform/Event.hpp"
#include "platform/Window.hpp"
#include "platform/input/Control.hpp"
#include "platform/input/Input.hpp"
#include "platform/input/InputAction.hpp"

using rtype::vulkan::platform::Event;
using namespace rtype::vulkan::platform::event;
using rtype::vulkan::platform::input::ActionType;
using rtype::vulkan::platform::input::Control;

int main() {
    try {
        /// @note Create a window with the specified width, height, and title
        rtype::vulkan::platform::Window window(800, 600, "Vulkan Window Example");
        rtype::vulkan::platform::input::Input input(window);

        /// @note Declare the actions once. Each one can be driven by any number of devices.

        /// @note Vector2: WASD, arrows and the left stick all move the player.
        auto& move = input.addAction("move", ActionType::Vector2)
                         .bindVector(Control::key(GLFW_KEY_W), Control::key(GLFW_KEY_S), Control::key(GLFW_KEY_A),
                                     Control::key(GLFW_KEY_D))
                         .bindVector(Control::key(GLFW_KEY_UP), Control::key(GLFW_KEY_DOWN),
                                     Control::key(GLFW_KEY_LEFT), Control::key(GLFW_KEY_RIGHT))
                         .bindVector(Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X),
                                     Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_Y));

        /// @note Button: space, left click or the A button.
        auto& fire = input.addAction("fire", ActionType::Button)
                         .bind(Control::key(GLFW_KEY_SPACE))
                         .bind(Control::mouseButton(GLFW_MOUSE_BUTTON_LEFT))
                         .bind(Control::gamepadButton(GLFW_GAMEPAD_BUTTON_A));

        /// @note Axis: Q / E or the gamepad triggers.
        auto& throttle = input.addAction("throttle", ActionType::Axis)
                             .bindAxis(Control::key(GLFW_KEY_Q), Control::key(GLFW_KEY_E))
                             .bindAxis(Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER),
                                       Control::gamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER));

        /// @note Vector2: mouse movement or the right stick.
        auto& look = input.addAction("look", ActionType::Vector2)
                         .bindVector(Control::mouseDeltaX(0.1F), Control::mouseDeltaY(0.1F))
                         .bindVector(Control::gamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_X),
                                     Control::gamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_Y));

        input.addAction("lock", ActionType::Button).bind(Control::key(GLFW_KEY_L));
        input.addAction("unlock", ActionType::Button).bind(Control::key(GLFW_KEY_U));

        while (window.isOpen()) {
            /// @note Poll for window events and update the window state
            for (const auto& event : window.pollEvents()) {
                /// @note Using the raw event system
                if (const auto* key = std::get_if<KeyPressed>(&event);
                    (key != nullptr) && key->key == GLFW_KEY_ESCAPE) {
                    std::cout << "Escape pressed, closing window.\n";
                    window.close();
                } else if (const auto* resized = std::get_if<Resized>(&event)) {
                    std::cout << "Window resized to " << resized->width << "x" << resized->height << "\n";
                } else if (const auto* text = std::get_if<TextEntered>(&event)) {
                    std::cout << "Text entered: " << static_cast<char>(text->codepoint) << "\n";
                }
            }

            /// @note Using action input system
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
                input.setCursorLocked(true);
            } else if (input.getAction("unlock").isPressed()) {
                input.setCursorLocked(false);
            }
            /// @}
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
