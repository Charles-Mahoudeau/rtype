/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Typed queries: a system asks for the entities that have some components, and gets their values with their C++
// types. Each term of Query<...> says what it needs: T (write), const T (read), Optional<T>, With<T>, Without<T>.

#include <exception>
#include <format>
#include <iostream>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;

namespace {
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

struct Health {
    int value;
};
RTYPE_ECS_COMPONENT(Health, "example.Health", value);

struct Shield {
    int strength;
};
RTYPE_ECS_COMPONENT(Shield, "example.Shield", strength);

/// @brief Tags: no data.
struct Player {};
RTYPE_ECS_COMPONENT(Player, "example.Player");

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "example.Dead");

/// @brief Position written (Mut<Position>), Velocity read (const Velocity&): only entities having both.
void move(Query<Position, const Velocity> query) {
    for (auto [entity, position, velocity] : query) {
        position->x += velocity.x;
        position->y += velocity.y;
        std::cout << std::format("  #{} moved to ({}, {})\n", entity.getIndex(), position.get().x, position.get().y);
    }
}

/// @brief Health read; Shield read if present (a const pointer, null when absent); dead entities skipped.
void showHealth(Query<const Health, Optional<const Shield>, Without<Dead>> query) {
    for (auto [entity, health, shield] : query) {
        const int armor = shield != nullptr ? shield->strength : 0;
        std::cout << std::format("  #{} health {}, armor {}\n", entity.getIndex(), health.value, armor);
    }
}

/// @brief With<Player>: the entity must be a player, but the tag gives no value.
void showPlayers(Query<const Health, With<Player>> query) {
    for (auto [entity, health] : query) {
        std::cout << std::format("  player #{} has {} health\n", entity.getIndex(), health.value);
    }
}

/// @brief Runs the example, printing each step.
void run() {
    World world;
    const Entity ship = world.spawn(Position{.x = 0.F, .y = 0.F}, Velocity{.x = 1.F, .y = 0.F}, Health{.value = 100},
                                    Shield{.strength = 25}, Player{});
    const Entity enemy =
        world.spawn(Position{.x = 50.F, .y = 10.F}, Velocity{.x = -2.F, .y = 1.F}, Health{.value = 30});
    const Entity rock = world.spawn(Position{.x = 20.F, .y = 20.F});
    const Entity wreck = world.spawn(Health{.value = 0}, Dead{});
    std::cout << std::format("ship #{}, enemy #{}, rock #{}, wreck #{}\n", ship.getIndex(), enemy.getIndex(),
                             rock.getIndex(), wreck.getIndex());

    std::cout << "\n== Query<Position, const Velocity>: the rock has no Velocity ==\n";
    System{world, "example.move", &move}.run();

    std::cout << "\n== Query<const Health, Optional<const Shield>, Without<Dead>> ==\n";
    System{world, "example.showHealth", &showHealth}.run();

    std::cout << "\n== Query<const Health, With<Player>> ==\n";
    System{world, "example.showPlayers", &showPlayers}.run();
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
