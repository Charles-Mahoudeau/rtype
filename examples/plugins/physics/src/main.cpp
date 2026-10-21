/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Runs a small physics scene with the physics plugin, headless, for three seconds, and prints it twice per second:
// - a ball falls under gravity and bounces on a static floor; its Transform and Velocity follow its body;
// - a kinematic ship flies at the speed its Velocity gives it, pushing nothing but never pushed;
// - a crate falls on the floor, then gameplay kicks it with an impulse after one second;
// - a fast bullet flies through the ship (their collision layers ignore each other) and stops on a target that only
//   bullets hit, which the ship flies through too.
// Gameplay only writes components (Transform, Collider, RigidBody, Velocity) and calls the PhysicsWorld resource.

#include <cstdint>
#include <exception>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <string>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

using namespace rtype::ecs;
using namespace rtype::plugins::physics;
using rtype::plugins::transform::Transform;

namespace {
constexpr std::uint64_t kTicksPerSecond = 60;
constexpr std::uint64_t kReportTicks = kTicksPerSecond / 2;  // Prints twice per second.
constexpr std::uint64_t kKickTick = kTicksPerSecond;         // The crate is kicked after one second...
constexpr std::uint64_t kExitTick = 3 * kTicksPerSecond;     // ...and the App stops after three.

// The game's collision categories: each Collider belongs to some (layer) and collides with some (mask).
constexpr std::uint32_t kScenery = 1U << 0;  // The floor, the ball, the crate.
constexpr std::uint32_t kShip = 1U << 1;
constexpr std::uint32_t kBullet = 1U << 2;
constexpr std::uint32_t kTarget = 1U << 3;

// The entities the gameplay systems talk about, set by the Startup system.
struct Scene {
    Entity ball;
    Entity ship;
    Entity crate;
    Entity bullet;
};
RTYPE_ECS_RESOURCE(Scene, "example.Scene");

// Startup: spawns the scene. Distances are in world units (pixels), Y down.
void spawnScene(Commands& commands, ResMut<Scene> scene) {
    // A static floor: a Collider without RigidBody. Its top is at y = 300.
    commands.spawn()
        .add(Transform{.position = {0.0F, 316.0F}})
        .add(Collider{.shape = ShapeType::kBox, .size = {2000.0F, 32.0F}});

    // A bouncy ball. With a Velocity, the physics writes its speed back every step.
    scene->ball = commands.spawn()
                      .add(Transform{.position = {0.0F, 0.0F}})
                      .add(Collider{.shape = ShapeType::kCircle, .radius = 16.0F, .restitution = 0.6F})
                      .add(RigidBody{})
                      .add(Velocity{})
                      .getEntity();

    // A kinematic ship: it flies at its Velocity, gravity and contacts never move it. It only collides with scenery.
    scene->ship =
        commands.spawn()
            .add(Transform{.position = {-400.0F, 100.0F}})
            .add(Collider{
                .shape = ShapeType::kCapsule, .radius = 8.0F, .halfHeight = 16.0F, .layer = kShip, .mask = kScenery})
            .add(RigidBody{.type = BodyType::kKinematic})
            .add(Velocity{.linear = {300.0F, 0.0F}})
            .getEntity();

    // A heavy crate that will be kicked.
    scene->crate = commands.spawn()
                       .add(Transform{.position = {200.0F, 200.0F}})
                       .add(Collider{.shape = ShapeType::kBox, .size = {48.0F, 48.0F}})
                       .add(RigidBody{.mass = 10.0F, .fixedRotation = true})
                       .getEntity();

    // A target in the ship's lane, hit only by bullets: the ship flies through it.
    commands.spawn()
        .add(Transform{.position = {400.0F, 100.0F}})
        .add(Collider{.shape = ShapeType::kBox, .size = {32.0F, 64.0F}, .layer = kTarget, .mask = kBullet});

    // A fast bullet behind the ship, in the same lane. It ignores the ship and the ship ignores it, so it flies
    // through; it hits scenery and targets. No gravity, and continuous collision so it never tunnels at this speed.
    scene->bullet =
        commands.spawn()
            .add(Transform{.position = {-700.0F, 100.0F}})
            .add(Collider{.shape = ShapeType::kCircle, .radius = 4.0F, .layer = kBullet, .mask = kScenery | kTarget})
            .add(RigidBody{.gravityScale = 0.0F, .fixedRotation = true, .continuousCollision = true})
            .add(Velocity{.linear = {600.0F, 0.0F}})
            .getEntity();
}

// FixedUpdate, after the physics wrote the Transforms back: prints the scene twice per second.
void report(Res<PhysicsWorld> world, Res<Scene> scene, Query<const Transform> transforms,
            Query<const Velocity> velocities) {
    if (world->getStepCount() % kReportTicks != 0) {
        return;
    }
    glm::vec2 ball{};
    glm::vec2 ship{};
    glm::vec2 crate{};
    glm::vec2 bullet{};
    for (auto [entity, transform] : transforms) {
        if (entity == scene->ball) {
            ball = transform.position;
        } else if (entity == scene->ship) {
            ship = transform.position;
        } else if (entity == scene->crate) {
            crate = transform.position;
        } else if (entity == scene->bullet) {
            bullet = transform.position;
        }
    }
    glm::vec2 ballSpeed{};
    for (auto [entity, velocity] : velocities) {
        if (entity == scene->ball) {
            ballSpeed = velocity.linear;
        }
    }
    std::cout << std::format(
        "t={:.1f}s  ball ({:5.1f}, {:5.1f}) speed ({:4.1f}, {:6.1f})  ship x {:6.1f}  bullet x {:6.1f}  "
        "crate ({:5.1f}, {:5.1f})  {} bodies\n",
        static_cast<float>(world->getStepCount()) / kTicksPerSecond, ball.x, ball.y, ballSpeed.x, ballSpeed.y, ship.x,
        bullet.x, crate.x, crate.y, world->getBodyCount());
}

// FixedUpdate, between the Transform sync and the step: kicks the crate once, then stops the App.
void script(ResMut<PhysicsWorld> world, Res<Scene> scene, EventWriter<AppExit> exit) {
    if (world->getStepCount() == kKickTick) {
        // An impulse is a mass times a change of speed: 10 kg at (200, -400) units/s, up and to the right.
        world->applyImpulse(scene->crate, {2000.0F, -4000.0F});
        std::cout << "crate kicked\n";
    }
    if (world->getStepCount() == kExitTick) {
        exit.send(AppExit{.code = 0});
    }
}
}  // namespace

int main() {
    try {
        App app;
        // MinimalPlugins brings Time (fixed ticks at 60 Hz), which the physics steps with.
        app.addPlugins(MinimalPlugins{});
        // 980 units/s^2 down is 9.8 m/s^2 at 100 units per meter. Invalid settings throw a PhysicsException here.
        app.addPlugin(PhysicsPlugin{PhysicsSettings{.gravity = {0.0F, 980.0F}, .unitsPerMeter = 100.0F}});
        app.insertResource(Scene{});
        app.addSystem(Phase::kStartup, "example.spawnScene", &spawnScene);
        // Systems order themselves around the physics systems by name.
        app.addSystem(Phase::kFixedUpdate, "example.script", &script)
            .after(std::string{PhysicsPlugin::kSyncToPhysicsSystem})
            .before(std::string{PhysicsPlugin::kStepSystem});
        app.addSystem(Phase::kFixedUpdate, "example.report", &report)
            .after(std::string{PhysicsPlugin::kSyncFromPhysicsSystem});
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "physics example: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "physics example: unknown error\n";
    }
    return 1;
}
