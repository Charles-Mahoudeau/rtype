/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// A boss made of parts, headless, for two seconds, printed twice a second:
// - the boss flies to the right; a turret, a static body, is its child, and a barrel is the turret's child: both follow
//   it, and their Transforms never change, since they are relative to their parent;
// - a pod is a dynamic child of the boss: the simulation owns where it is, so it stays in place as the boss flies away,
//   and its Transform changes instead;
// - after one second the pod is detached in place: it keeps its position, and its Transform becomes a world position;
// - after a second and a half the boss is despawned: the turret and the barrel go with it, the detached pod stays.
//   xmake f --hierarchy=y && xmake run hierarchy

#include <cstdint>
#include <exception>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <string>
#include <string_view>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/transform/Reparent.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

using namespace rtype::ecs;
using namespace rtype::plugins::physics;
using namespace rtype::plugins::transform;

namespace {
constexpr std::uint64_t kPrintEvery = 30;
constexpr std::uint64_t kDetachTick = 60;
constexpr std::uint64_t kDespawnTick = 90;
constexpr std::uint64_t kExitTick = 120;
constexpr float kBossSpeed = 2.0F;  // Units per tick.

/// @brief The parts of the boss.
struct Boss {
    Entity boss;
    Entity turret;
    Entity barrel;
    Entity pod;
    std::uint64_t printed{0};  // The last tick printed.
};
RTYPE_ECS_RESOURCE(Boss, "example.Boss");

// Startup: `add(Parent{...})` links two entities; the World keeps the parent's Children in sync.
void spawnBoss(ResMut<Boss> scene, Commands& commands) {
    scene->boss = commands.spawn().add(Transform{.position = {100.0F, 200.0F}}).getEntity();
    scene->turret = commands.spawn()
                        .add(Transform{.position = {0.0F, -40.0F}})
                        .add(Collider{.shape = ShapeType::kCircle, .radius = 10.0F})
                        .add(Parent{.entity = scene->boss})
                        .getEntity();
    scene->barrel =
        commands.spawn().add(Transform{.position = {25.0F, 0.0F}}).add(Parent{.entity = scene->turret}).getEntity();
    scene->pod = commands.spawn()
                     .add(Transform{.position = {0.0F, 60.0F}})
                     .add(Collider{.shape = ShapeType::kCircle, .radius = 8.0F})
                     .add(RigidBody{.type = BodyType::kDynamic})
                     .add(Parent{.entity = scene->boss})
                     .getEntity();
}

// FixedUpdate, between the physics systems: gameplay only moves the root, the parts follow.
void flyBoss(Res<Boss> scene, Query<Transform> transforms) {
    for (auto [entity, transform] : transforms) {
        if (entity == scene->boss) {
            transform->position.x += kBossSpeed;
        }
    }
}

// FixedUpdate: what happens to the boss over time, by tick.
void script(Res<PhysicsWorld> world, Res<Boss> scene, Commands& commands, Query<const GlobalTransform> globals,
            EventWriter<AppExit> exit) {
    const std::uint64_t tick = world->getStepCount();
    if (tick == kDetachTick) {
        for (auto [entity, global] : globals) {
            if (entity == scene->pod) {
                detachInPlace(commands, entity, global);  // `remove<Parent>()` alone would make it jump
                std::cout << "the pod is detached\n";
            }
        }
    }
    if (tick == kDespawnTick) {
        commands.despawn(scene->boss);
        std::cout << "the boss is despawned, with its children\n";
    }
    if (tick == kExitTick) {
        exit.send(AppExit{.code = 0});
    }
}

/// @return A line about @p entity, or an empty string if it is gone.
std::string describe(std::string_view name, Entity entity, Query<const Transform, const GlobalTransform>& placed) {
    for (auto [other, local, global] : placed) {
        if (other == entity) {
            return std::format("  {:<7} local ({:7.1f}, {:7.1f})   world ({:7.1f}, {:7.1f})\n", name, local.position.x,
                               local.position.y, global.position.x, global.position.y);
        }
    }
    return std::format("  {:<7} gone\n", name);
}

// PostUpdate, once the GlobalTransforms are computed: prints the scene.
void report(Res<PhysicsWorld> world, ResMut<Boss> scene, Query<const Transform, const GlobalTransform> placed) {
    const std::uint64_t tick = world->getStepCount();
    if (tick == 0 || tick % kPrintEvery != 0 || tick == scene->printed) {
        return;
    }
    scene->printed = tick;
    std::cout << std::format("tick {}\n", tick) << describe("boss", scene->boss, placed)
              << describe("turret", scene->turret, placed) << describe("barrel", scene->barrel, placed)
              << describe("pod", scene->pod, placed);
}
}  // namespace

int main() {
    try {
        App app;
        app.addPlugins(MinimalPlugins{});
        app.addPlugin(rtype::plugins::transform::TransformPlugin{});
        app.addPlugin(PhysicsPlugin{PhysicsSettings{.gravity = {0.0F, 0.0F}, .unitsPerMeter = 100.0F}});
        app.insertResource(Boss{});
        app.addSystem(Phase::kStartup, "example.spawnBoss", &spawnBoss);
        app.addSystem(Phase::kFixedUpdate, "example.flyBoss", &flyBoss)
            .after(std::string{PhysicsPlugin::kSyncBodiesSystem})
            .before(std::string{PhysicsPlugin::kSyncToPhysicsSystem});
        app.addSystem(Phase::kFixedUpdate, "example.script", &script).after(std::string{PhysicsPlugin::kEventsSystem});
        app.addSystem(Phase::kPostUpdate, "example.report", &report)
            .after(std::string{TransformPlugin::kPropagateSystem});
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "hierarchy example: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "hierarchy example: unknown error\n";
    }
    return 1;
}
