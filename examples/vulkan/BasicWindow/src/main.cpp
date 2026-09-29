/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <GLFW/glfw3.h>

#include <cstdio>
#include <iostream>

#include "platform/input/Input.hpp"
#include "platform/Window.hpp"

using rtype::vulkan::platform::input::ActionType;
using rtype::vulkan::platform::input::Control;

int main() {
    // Create a window with the specified width, height, and title
    rtype::vulkan::platform::Window window(800, 600, "Vulkan Window Example");
    rtype::vulkan::platform::input::Input input(window);

    // Declare the actions once. Each one can be driven by any number of devices.

    // Vector2: WASD, arrows and the left stick all move the player.
    auto& move = input.addAction("move", ActionType::Vector2)
                     .bindVector(Control::key(GLFW_KEY_W), Control::key(GLFW_KEY_S), Control::key(GLFW_KEY_A),
                                 Control::key(GLFW_KEY_D))
                     .bindVector(Control::key(GLFW_KEY_UP), Control::key(GLFW_KEY_DOWN), Control::key(GLFW_KEY_LEFT),
                                 Control::key(GLFW_KEY_RIGHT))
                     .bindVector(Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_X),
                                 Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_Y));

    // Button: space, left click or the A button.
    auto& fire = input.addAction("fire", ActionType::Button)
                     .bind(Control::key(GLFW_KEY_SPACE))
                     .bind(Control::mouseButton(GLFW_MOUSE_BUTTON_LEFT))
                     .bind(Control::gamepadButton(GLFW_GAMEPAD_BUTTON_A));

    // Axis: Q / E or the gamepad triggers.
    auto& throttle = input.addAction("throttle", ActionType::Axis)
                         .bindAxis(Control::key(GLFW_KEY_Q), Control::key(GLFW_KEY_E))
                         .bindAxis(Control::gamepadAxis(GLFW_GAMEPAD_AXIS_LEFT_TRIGGER),
                                   Control::gamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER));

    // Vector2: mouse movement or the right stick.
    auto& look = input.addAction("look", ActionType::Vector2)
                     .bindVector(Control::mouseDeltaX(0.1f), Control::mouseDeltaY(0.1f))
                     .bindVector(Control::gamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_X),
                                 Control::gamepadAxis(GLFW_GAMEPAD_AXIS_RIGHT_Y));

    input.addAction("lock", ActionType::Button).bind(Control::key(GLFW_KEY_L));
    input.addAction("unlock", ActionType::Button).bind(Control::key(GLFW_KEY_U));

    // Poll for window events until the user closes the window
    while (window.isOpen()) {
        window.pollEvents();
        input.update();

        if (auto dir = move.readVector(); dir.x != 0.0F || dir.y != 0.0F) {
            std::cout << "Move: (" << dir.x << ", " << dir.y << ")\n";
        }
        if (fire.isPressed()) {
            std::cout << "Fire!\n";
        }
        if (float value = throttle.readAxis(); value != 0.0F) {
            std::cout << "Throttle: " << value << "\n";
        }
        if (auto delta = look.readVector(); delta.x != 0.0F || delta.y != 0.0F) {
            std::cout << "Look: (" << delta.x << ", " << delta.y << ")\n";
        }

        // Actions can also be fetched by name.
        if (input.getAction("lock").isPressed()) {
            input.setCursorLocked(true);
        } else if (input.getAction("unlock").isPressed()) {
            input.setCursorLocked(false);
        }
    }
    return 0;
}
