/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Entities: what an Entity is (an index and a generation), how to spawn and despawn them, and why an old handle
// can never reach the entity that reuses its slot.

#include <exception>
#include <format>
#include <iostream>
#include <string>

#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
/// @return "#index v generation", the two halves of an Entity.
std::string describe(Entity entity) { return std::format("#{} v{}", entity.getIndex(), entity.getGeneration()); }

/// @brief Runs the example, printing each step.
void run() {
    World world;

    std::cout << "== Spawning ==\n";
    const Entity ship = world.spawn();
    const Entity enemy = world.spawn();
    std::cout << std::format("ship {}, enemy {}, {} alive\n", describe(ship), describe(enemy), world.getEntityCount());

    std::cout << "\n== Despawning ==\n";
    world.despawn(ship);
    std::cout << std::format("ship alive? {}, {} alive\n", world.isAlive(ship), world.getEntityCount());

    std::cout << "\n== Reusing a slot ==\n";
    // The freed index is reused, with its generation bumped: the new entity is a different handle.
    const Entity bullet = world.spawn();
    std::cout << std::format("bullet {} (same index as the ship, next generation)\n", describe(bullet));
    std::cout << std::format("old ship handle alive? {}; bullet alive? {}; same handle? {}\n", world.isAlive(ship),
                             world.isAlive(bullet), ship == bullet);

    std::cout << "\n== A stale handle does nothing ==\n";
    // Despawning through the old handle is refused: it cannot destroy the bullet now living in that slot.
    std::cout << std::format("despawn(old ship) -> {}; bullet still alive? {}\n", world.despawn(ship),
                             world.isAlive(bullet));
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
