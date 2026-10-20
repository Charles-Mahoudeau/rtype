/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Runs an App with the physics plugin, headless, for three seconds: the plugin steps its PhysicsWorld once per fixed
// tick (60 per second), and two gameplay systems read and change the world through the resource.

#include <cstdint>
#include <exception>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <string>

#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

using namespace rtype::ecs;
using namespace rtype::plugins::physics;

namespace {
constexpr std::uint64_t kTicksPerSecond = 60;
constexpr std::uint64_t kFlipTick = 2 * kTicksPerSecond;  // Gravity turns upward after two seconds...
constexpr std::uint64_t kExitTick = 3 * kTicksPerSecond;  // ...and the App stops after three.

// FixedUpdate, after the step: prints the world once per second.
void report(Res<PhysicsWorld> world) {
    if (world->getStepCount() % kTicksPerSecond != 0) {
        return;
    }
    const glm::vec2 gravity = world->getGravity();
    std::cout << std::format("step {:>3}: gravity ({}, {}) units/s^2, {} bodies, {} units per meter\n",
                             world->getStepCount(), gravity.x, gravity.y, world->getBodyCount(),
                             world->getUnitsPerMeter());
}

// FixedUpdate, after the step: gameplay changes the world through ResMut, then stops the App.
void script(ResMut<PhysicsWorld> world, EventWriter<AppExit> exit) {
    if (world->getStepCount() == kFlipTick) {
        const glm::vec2 gravity = world->getGravity();
        world->setGravity({gravity.x, -gravity.y});
        std::cout << "gravity flipped\n";
    }
    if (world->getStepCount() == kExitTick) {
        exit.send(AppExit{.code = 0});
    }
}
}  // namespace

int main() {
    try {
        App app;
        // MinimalPlugins brings Time (fixed ticks at 60 Hz), which the step system needs.
        app.addPlugins(MinimalPlugins{});
        // Distances are in world units (pixels): 980 units/s^2 down is 9.8 m/s^2 at 100 units per meter. Invalid
        // settings throw a PhysicsException here.
        app.addPlugin(PhysicsPlugin{PhysicsSettings{.gravity = {0.0F, 980.0F}, .unitsPerMeter = 100.0F}});
        // Systems that read the result of a step order themselves after it.
        app.addSystem(Phase::kFixedUpdate, "example.report", &report).after(std::string{PhysicsPlugin::kStepSystem});
        app.addSystem(Phase::kFixedUpdate, "example.script", &script).after("example.report");
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "physics example: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "physics example: unknown error\n";
    }
    return 1;
}
