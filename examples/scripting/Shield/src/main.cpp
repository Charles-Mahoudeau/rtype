/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// A game made of Luau files: the ScriptingPlugin loads every file of game/scripts, in sorted path order, and the
// components and systems they declare join the App. The program only creates the entity the scripts start from.

#include <chrono>
#include <exception>
#include <iostream>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/scripting/plugin/ScriptingPlugin.hpp"

using namespace rtype;

int main() {
    try {
        ecs::App app;
        // One fixed tick per frame, one second long.
        app.addPlugin(ecs::TimePlugin::manual(std::chrono::seconds{1}, ecs::FixedTimestep{1, 5}));
        app.addPlugin(scripting::ScriptingPlugin{{
            .directory = "examples/scripting/Shield/game/scripts",
            .options = {.isClient = true, .isServer = true},
        }});

        auto& components = app.getWorld().getComponents();
        components.add(app.getWorld().spawn(), *components.find("game.Spawner"), {});

        for (int frame = 0; frame < 4; ++frame) {
            std::cout << "frame " << frame << '\n';
            app.runFrame();
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
