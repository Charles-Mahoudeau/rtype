/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Prefabs: named templates of entities. A prefab holds component values prepared once and child prefabs; spawning
// it copies them into new entities, the children parented to the root. Overrides are plain adds after the spawn.
// Components of the other side of the network are left out, by the App's run mode.

#include <exception>
#include <format>
#include <iostream>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/exceptions/PrefabException.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/prefab/PrefabConfig.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
struct Position {
    float x;
};
RTYPE_ECS_COMPONENT(Position, "example.Position", x);

struct Health {
    int value;
};
RTYPE_ECS_COMPONENT(Health, "example.Health", value);

/// @brief Server-only: the AI state never reaches a client.
struct Brain {
    int state;
};
RTYPE_ECS_COMPONENT(Brain, "example.Brain", state);

/// @brief Prints an entity's components among the example's, and its parent.
void describe(const World& world, std::string_view label, Entity entity) {
    const Components& components = world.getComponents();
    std::cout << std::format("  {} {}:", label, entity.getIndex());
    if (const auto* position = components.get<Position>(entity)) {
        std::cout << std::format(" Position {}", position->x);
    }
    if (const auto* health = components.get<Health>(entity)) {
        std::cout << std::format(" Health {}", health->value);
    }
    if (components.has<Brain>(entity)) {
        std::cout << " Brain";
    }
    if (const auto* parent = components.get<Parent>(entity)) {
        std::cout << std::format(" (child of {})", parent->entity.getIndex());
    }
    std::cout << "\n";
}

/// @brief Registers the example's prefabs in the App: a turret, and a boss with two turrets.
void addPrefabs(App& app) {
    static_cast<void>(app.getWorld().getComponents().registerType<Brain>({.presence = Presence::kServerOnly}));
    app.addPrefab("example.Turret").with(Position{.x = 0.F}).with(Health{.value = 5});
    app.addPrefab("example.Boss")
        .with(Position{.x = 0.F})
        .with(Health{.value = 50})
        .with(Brain{.state = 1})
        .withChild("example.Turret", Position{.x = -40.F})  // each child with its own position
        .withChild("example.Turret", Position{.x = 40.F});
}

/// @brief Spawns a boss in the App's world and prints the tree.
void spawnBoss(App& app, std::string_view side) {
    Commands commands{app.getWorld()};
    const Entity boss = commands.spawnPrefab("example.Boss").add(Position{.x = 820.F}).getEntity();  // an override
    commands.flush();
    std::cout << std::format("== A boss spawned on the {} ==\n", side);
    describe(app.getWorld(), "boss", boss);
    for (const Entity turret : app.getWorld().getComponents().get<Children>(boss)->entities) {
        describe(app.getWorld(), "turret", turret);
    }
}

/// @brief Runs the example, printing each step.
void run() {
    App server{RunMode::kServer};
    addPrefabs(server);
    spawnBoss(server, "server");

    std::cout << "\n";
    App client{RunMode::kClient};
    addPrefabs(client);
    spawnBoss(client, "client (no Brain: it is server-only)");

    std::cout << "\n== Mistakes are refused ==\n";
    try {
        client.addPrefab("example.Loop").withChild("example.Loop");
    } catch (const exceptions::PrefabException& error) {
        std::cout << std::format("  {}\n", error.what());
    }
    try {
        static_cast<void>(client.getWorld().spawnPrefab("example.Missing"));
    } catch (const exceptions::PrefabException& error) {
        std::cout << std::format("  {}\n", error.what());
    }
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
