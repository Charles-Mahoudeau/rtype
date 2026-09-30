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
#include "engine/input/Input.hpp"
#include "engine/input/Key.hpp"
#include "platform/Window.hpp"

int main() {
    try {
        rtype::platform::Window window(1280, 720, "R-Type");
        rtype::engine::input::Input input;

        while (window.isOpen()) {
            for (const auto& event : window.pollEvents()) {
                input.handleEvent(event);
                if (const auto* key = std::get_if<rtype::engine::event::KeyPressed>(&event);
                    key != nullptr && key->key == rtype::engine::input::Key::kEscape) {
                    window.close();
                }
            }
            input.update();
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
