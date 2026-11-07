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
#include <cstdint>
#include <exception>
#include <format>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/CapsuleSpawn.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
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

/// @return A new child of `parent` in `capsule`, which spawns the capsule at `path` if it is not empty.
std::uint32_t addChild(Capsule& capsule, std::uint32_t parent, std::string name, std::string_view path = {}) {
    const std::uint32_t node = capsule.addNode(std::move(name), parent);
    if (!path.empty()) {
        capsule.editNode(node).instance = CapsuleRef{.id = CapsuleId::fromPath(path), .path = std::string{path}};
    }
    return node;
}

/// @brief Defines the turret, with `health`.
void defineTurret(App& app, int health) {
    Capsule turret{"actors/turret.actor"};
    turret.editNode(0).values.set(app.getWorld().getComponents(), Health{.value = health});
    app.getWorld().getResources().getMut<Capsules>()->define(std::move(turret));
}

/// @brief Adds the plugin and the capsules: a turret, a boss with two turrets, and two levels.
void addCapsules(App& app) {
    app.addPlugin(CapsulePlugin{});
    Components& components = app.getWorld().getComponents();
    static_cast<void>(components.registerType<Brain>({.presence = Presence::kServerOnly}));
    Capsules& capsules = *app.getWorld().getResources().getMut<Capsules>();
    defineTurret(app, 5);

    Capsule boss{"actors/boss.actor"};
    boss.editNode(0).values.set(components, Health{.value = 50});
    boss.editNode(0).values.set(components, Brain{.state = 1});
    boss.editNode(addChild(boss, 0, "LeftTurret", "actors/turret.actor")).values.set(components, Position{.x = -40.F});
    boss.editNode(addChild(boss, 0, "RightTurret", "actors/turret.actor")).values.set(components, Position{.x = 40.F});
    capsules.define(std::move(boss));

    Capsule level1{"scenes/level1.scene"};
    level1.editNode(addChild(level1, 0, "Boss", "actors/boss.actor")).values.set(components, Position{.x = 600.F});
    CapsuleNode& background = level1.editNode(addChild(level1, 0, "Background"));
    background.side = Side::kClient;
    background.values.set(components, Background{.layer = 0});
    capsules.define(std::move(level1));

    Capsule level2{"scenes/level2.scene"};
    level2.editNode(addChild(level2, 0, "Turret", "actors/turret.actor")).values.set(components, Position{.x = 300.F});
    capsules.define(std::move(level2));
}

/// @return The root of the capsule at `path`, loaded in place of the one loaded before, as a system would.
Entity load(App& app, std::string_view path) {
    World& world = app.getWorld();
    Commands commands{world};
    const Entity root = loadCapsule(commands, *world.getResources().getMut<Capsules>(),
                                    *world.getResources().getMut<LoadedCapsule>(), CapsuleId::fromPath(path))
                            .getEntity();
    commands.flush();
    return root;
}

/// @brief Runs the example, printing each step.
void run() {
    App server{RunMode::kServer};
    addCapsules(server);
    std::cout << "== Level 1 on the server (no background) ==\n";
    describe(server.getWorld(), load(server, "scenes/level1.scene"));

    App client{RunMode::kClient};
    addCapsules(client);
    std::cout << "\n== Level 1 on a client (the boss comes from the server) ==\n";
    describe(client.getWorld(), load(client, "scenes/level1.scene"));

    World& world = server.getWorld();
    std::cout << "\n== Switching to level 2 ==\n";
    const Entity level2 = load(server, "scenes/level2.scene");
    describe(world, level2);
    std::cout << std::format("  {} entities left in the world\n", world.getEntityCount());

    std::cout << "\n== The turret, defined again, reloaded into the level that holds it ==\n";
    defineTurret(server, 9);
    Commands commands{world};
    const auto* children = world.getComponents().get<Children>(level2);
    reloadCapsule(commands, world.getResources().require<Capsules>(), level2,
                  *world.getComponents().get<CapsuleInstance>(level2),
                  children != nullptr ? std::vector<Entity>{children->entities} : std::vector<Entity>{});
    commands.flush();
    describe(world, level2);
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
