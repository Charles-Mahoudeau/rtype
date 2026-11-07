/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// A game whose only content is its user interface, in game/scripts/menu.luau (its `scripts.main` entry). No C++ knows the UI: the Engine loads the
// plugins project.luau names, as for any game.

#include <exception>
#include <iostream>

#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"

int main() {
    try {
        rtype::engine::Engine engine{{.mode = rtype::ecs::RunMode::kStandalone, .game = "examples/plugins/ui/game"}};
        return engine.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}
