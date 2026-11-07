/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Capsules: an entity described once (components and child entities, a child can be another capsule) and spawned by
// copy. A boss with two turrets and a level holding the boss are the same thing; each node is spawned on its side, so
// the same level spawns on a server and a client without duplicates. Loading switches levels; a capsule defined again
// is reloaded into what was spawned from it.

#include <cstddef>
#include <exception>
#include <format>
#include <iostream>
#include <string>
#include <string_view>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/NodeConfig.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

using namespace rtype::ecs;
using namespace rtype::plugins::capsule;

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

/// @brief What a client draws behind the level.
struct Background {
    int layer;
};
RTYPE_ECS_COMPONENT(Background, "example.Background", layer);

/// @brief Prints an entity and its descendants, indented, with the example's components.
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the tree.
void describe(const World& world, Entity entity, std::size_t depth = 1) {
    const Components& components = world.getComponents();
    std::cout << std::format("{:{}}{}:", "", depth * 2, entity.getIndex());
    if (const auto* position = components.get<Position>(entity)) {
        std::cout << std::format(" Position {}", position->x);
    }
    if (const auto* health = components.get<Health>(entity)) {
        std::cout << std::format(" Health {}", health->value);
    }
    if (components.has<Brain>(entity)) {
        std::cout << " Brain";
    }
    if (components.has<Background>(entity)) {
        std::cout << " Background";
    }
    if (const auto* instance = components.get<CapsuleInstance>(entity)) {
        std::cout << std::format(" (capsule v{}{})", instance->version, instance->nested ? ", nested" : "");
    }
    std::cout << "\n";
    if (const auto* children = components.get<Children>(entity)) {
        for (const Entity child : children->entities) {
            describe(world, child, depth + 1);  // NOLINT(misc-no-recursion)
        }
    }
}

/// @brief Adds the plugin and the capsules: a turret, a boss with two turrets, and two levels.
void addCapsules(App& app) {
    app.addPlugin(CapsulePlugin{});
    static_cast<void>(app.getWorld().getComponents().registerType<Brain>({.presence = Presence::kServerOnly}));
    addCapsule(app, "actors/turret.actor").with(Health{.value = 5});
    NodeConfig boss = addCapsule(app, "actors/boss.actor");
    boss.with(Health{.value = 50}).with(Brain{.state = 1});
    boss.child("LeftTurret").instance("actors/turret.actor").with(Position{.x = -40.F});
    boss.child("RightTurret").instance("actors/turret.actor").with(Position{.x = 40.F});
    NodeConfig level1 = addCapsule(app, "scenes/level1.scene");
    level1.child("Boss").instance("actors/boss.actor").with(Position{.x = 600.F});
    level1.child("Background").onSide(Side::kClient).with(Background{.layer = 0});
    addCapsule(app, "scenes/level2.scene").child("Turret").instance("actors/turret.actor").with(Position{.x = 300.F});
}

/// @brief Runs the example, printing each step.
void run() {
    App server{RunMode::kServer};
    addCapsules(server);
    std::cout << "== Level 1 on the server (no background) ==\n";
    describe(server.getWorld(), loadCapsule(server.getWorld(), "scenes/level1.scene"));

    App client{RunMode::kClient};
    addCapsules(client);
    std::cout << "\n== Level 1 on a client (the boss comes from the server) ==\n";
    describe(client.getWorld(), loadCapsule(client.getWorld(), "scenes/level1.scene"));

    World& world = server.getWorld();
    std::cout << "\n== Switching to level 2 ==\n";
    const Entity level2 = loadCapsule(world, "scenes/level2.scene");
    describe(world, level2);
    std::cout << std::format("  {} entities left in the world\n", world.getEntityCount());

    std::cout << "\n== The turret, defined again, reloaded into what was spawned from it ==\n";
    addCapsule(server, "actors/turret.actor").with(Health{.value = 9});
    const std::size_t reloaded = reloadInstances(world, CapsuleId::fromPath("actors/turret.actor"));
    describe(world, level2);
    std::cout << std::format("  {} root(s) reloaded\n", reloaded);
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
