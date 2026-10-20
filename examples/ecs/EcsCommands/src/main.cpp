/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Commands: structural changes (spawn, add, remove, despawn) recorded by a system and applied later, at the flush,
// because doing them while iterating would move the values being read.

#include <exception>
#include <format>
#include <iostream>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
struct Health {
    int value;
};
RTYPE_ECS_COMPONENT(Health, "example.Health", value);

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "example.Dead");

/// @brief Marks the entities without health as dead, and spawns an explosion for each: both are commands.
void killTheDying(Query<const Health> query, Commands& commands) {
    for (auto [entity, health] : query) {
        if (health.value <= 0) {
            commands.add(entity, Dead{});
            const Entity explosion = commands.spawn().add(Health{.value = 1}).getEntity();
            std::cout << std::format("  recorded: #{} is dead, explosion #{} reserved\n", entity.getIndex(),
                                     explosion.getIndex());
        }
    }
}

/// @brief Despawns the dead.
void removeTheDead(Query<With<Dead>> query, Commands& commands) {
    for (auto [entity] : query) {
        commands.despawn(entity);
        std::cout << std::format("  recorded: despawn #{}\n", entity.getIndex());
    }
}

/// @brief Runs the example, printing each step.
void run() {
    World world;
    const Components& components = world.getComponents();
    const Entity ship = world.spawn(Health{.value = 100});
    const Entity enemy = world.spawn(Health{.value = 0});
    std::cout << std::format("ship #{}, enemy #{} (no health left), {} entities\n", ship.getIndex(), enemy.getIndex(),
                             world.getEntityCount());

    System kill{world, "example.killTheDying", &killTheDying};
    System remove{world, "example.removeTheDead", &removeTheDead};

    std::cout << "\n== A system records commands ==\n";
    kill.run();
    std::cout << std::format("before the flush: enemy dead? {}; {} entities\n", components.has<Dead>(enemy),
                             world.getEntityCount());

    std::cout << "\n== The flush applies them, in order (the schedules do it after each phase) ==\n";
    kill.flushCommands();
    std::cout << std::format("after the flush: enemy dead? {}; {} entities\n", components.has<Dead>(enemy),
                             world.getEntityCount());

    std::cout << "\n== Despawning is a command too ==\n";
    remove.run();
    remove.flushCommands();
    std::cout << std::format("enemy alive? {}; ship alive? {}; {} entities\n", world.isAlive(enemy),
                             world.isAlive(ship), world.getEntityCount());
}
}  // namespace

int main() {
    try {
        run();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
