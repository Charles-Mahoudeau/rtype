/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Components: declaring C++ components (with reflected fields, or as tags), adding, reading, writing and removing
// them, and listing what the registry knows about a component.

#include <exception>
#include <format>
#include <iostream>

#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
/// @brief A component with data: its fields are listed after the name, so Luau and the network can see them.
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "example.Position", x, y);

struct Velocity {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Velocity, "example.Velocity", x, y);

/// @brief A tag: no data, only "this entity is a player".
struct Player {};
RTYPE_ECS_COMPONENT(Player, "example.Player");

/// @brief Runs the example, printing each step.
void run() {
    World world;
    Components& components = world.getComponents();

    std::cout << "== Spawning with components ==\n";
    const Entity ship = world.spawn(Position{.x = 0.F, .y = 0.F}, Velocity{.x = 1.F, .y = 2.F}, Player{});
    std::cout << std::format("has Position? {}, Velocity? {}, Player? {}\n", components.has<Position>(ship),
                             components.has<Velocity>(ship), components.has<Player>(ship));

    std::cout << "\n== Reading ==\n";
    const auto* position = components.get<Position>(ship);  // nullptr if absent
    std::cout << std::format("position = ({}, {})\n", position->x, position->y);

    std::cout << "\n== Writing ==\n";
    // getMut returns a Mut<T>: writing through it marks the component changed (see EcsChangeDetection).
    Mut<Position> moved = components.getMut<Position>(ship);
    moved->x = 5.F;
    std::cout << std::format("position.x is now {}\n", components.get<Position>(ship)->x);

    std::cout << "\n== Adding and removing ==\n";
    const Entity rock = world.spawn();
    components.add(rock, Position{.x = 10.F, .y = 3.F});
    components.remove<Velocity>(ship);
    std::cout << std::format("rock has Position? {}; ship has Velocity? {} (get -> {})\n",
                             components.has<Position>(rock), components.has<Velocity>(ship),
                             components.get<Velocity>(ship) == nullptr ? "nullptr" : "a value");

    std::cout << "\n== What the registry knows ==\n";
    const ComponentInfo* info = components.getRegistry().find<Position>();
    std::cout << std::format("'{}': id {}, {} bytes\n", info->type.name, info->type.id, info->type.size);
    for (const FieldInfo& field : info->type.fields) {
        std::cout << std::format("  field '{}' at offset {}\n", field.name, field.offset);
    }
    const ComponentInfo* tag = components.getRegistry().find<Player>();
    std::cout << std::format("'{}': {} bytes (a tag)\n", tag->type.name, tag->type.size);
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
