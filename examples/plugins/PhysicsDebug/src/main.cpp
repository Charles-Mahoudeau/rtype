/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Opens a window rendered with Vulkan on a physics scene that has no sprites at all: everything seen is the physics
// debug view. Balls of every shape fall and bounce on a floor, a kinematic ship flies across at its Velocity, and a
// sensor zone hangs in the middle. Each entity gets a PhysicsDebug component, so its collider is outlined and its speed
// drawn as an arrow. F3 toggles the view; close the window to quit.
//
// Options: --validate turns the Vulkan validation layer on; --seconds N quits after N seconds.

#include <array>
#include <charconv>
#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "platform/glfw/Registration.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/config/Settings.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/debug/PhysicsDebug.hpp"
#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

using namespace rtype;
using namespace rtype::plugins::physics;
using rtype::plugins::backend::graphics::Color;
using rtype::plugins::transform::Transform;

namespace {
constexpr double kWidth = 800.0;
constexpr double kHeight = 600.0;
constexpr Color kOrange{1.0F, 0.6F, 0.1F, 1.0F};
constexpr Color kBlue{0.4F, 0.6F, 1.0F, 1.0F};
constexpr Color kYellow{1.0F, 0.9F, 0.2F, 1.0F};

// How long the demo runs, if --seconds was given, and how long it has run.
struct Lifetime {
    std::optional<float> seconds;
    float elapsed = 0.0F;
};
RTYPE_ECS_RESOURCE(Lifetime, "example.Lifetime");

// Startup: the scene, every entity with its debug drawing switched on. Distances are in world units, Y down.
void spawnScene(ecs::Commands& commands) {
    // A static floor and two walls: colliders without RigidBody, drawn in the default green.
    commands.spawn()
        .add(Transform{.position = {400.0F, 560.0F}})
        .add(Collider{.shape = ShapeType::kBox, .size = {760.0F, 20.0F}})
        .add(debug::PhysicsDebug{});
    commands.spawn()
        .add(Transform{.position = {30.0F, 400.0F}, .rotation = 0.3F})
        .add(Collider{.shape = ShapeType::kBox, .size = {20.0F, 300.0F}})
        .add(debug::PhysicsDebug{});
    commands.spawn()
        .add(Transform{.position = {770.0F, 400.0F}, .rotation = -0.3F})
        .add(Collider{.shape = ShapeType::kBox, .size = {20.0F, 300.0F}})
        .add(debug::PhysicsDebug{});

    // Bouncy bodies of each shape, falling from different heights; their Velocity draws their arrow.
    const std::array shapes{
        Collider{.shape = ShapeType::kCircle, .radius = 20.0F, .restitution = 0.7F},
        Collider{.shape = ShapeType::kBox, .size = {40.0F, 30.0F}, .restitution = 0.5F},
        Collider{.shape = ShapeType::kCapsule, .radius = 12.0F, .halfHeight = 18.0F, .restitution = 0.6F},
    };
    for (std::size_t index = 0; index < 6; ++index) {
        commands.spawn()
            .add(Transform{.position = {180.0F + (90.0F * static_cast<float>(index)),
                                        80.0F + (40.0F * static_cast<float>(index % 3))},
                           .rotation = 0.2F * static_cast<float>(index)})
            .add(shapes.at(index % shapes.size()))
            .add(RigidBody{})
            .add(Velocity{.linear = {index % 2 == 0 ? 120.0F : -120.0F, 0.0F}})
            .add(debug::PhysicsDebug{.color = kOrange});
    }

    // A kinematic ship flying across at its Velocity, pushing the balls in its way.
    commands.spawn()
        .add(Transform{.position = {-60.0F, 470.0F}})
        .add(Collider{.shape = ShapeType::kCapsule, .radius = 14.0F, .halfHeight = 20.0F})
        .add(RigidBody{.type = BodyType::kKinematic})
        .add(Velocity{.linear = {90.0F, 0.0F}})
        .add(debug::PhysicsDebug{.color = kBlue});

    // A sensor zone in the middle: drawn fainter, it detects without colliding.
    commands.spawn()
        .add(Transform{.position = {400.0F, 300.0F}})
        .add(Collider{.shape = ShapeType::kCircle, .radius = 70.0F, .sensor = true})
        .add(debug::PhysicsDebug{.color = kYellow});
}

// Update: quits after --seconds, if given.
void quitWhenDone(ecs::Res<ecs::Time> time, ecs::ResMut<Lifetime> lifetime, ecs::EventWriter<ecs::AppExit> exit) {
    lifetime->elapsed += time->delta;
    const std::optional<float> limit = lifetime.get().seconds;
    if (limit && lifetime.get().elapsed >= *limit) {
        exit.send(ecs::AppExit{.code = 0});
    }
}

// The window, the renderer and, when asked, the validation layer.
plugins::backend::config::Settings makeSettings(bool validate) {
    plugins::backend::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "Physics debug view (F3 toggles)");
    settings.set("window.size", std::vector<double>{kWidth, kHeight});
    settings.set("vulkan.virtualResolution", std::vector<double>{kWidth, kHeight});
    if (validate) {
        settings.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
        settings.set("vulkan.debugging", true);
    }
    return settings;
}
}  // namespace

int main(int argc, char** argv) {
    bool validate = false;
    Lifetime lifetime;
    const std::span<char*> arguments{argv, static_cast<std::size_t>(argc)};
    for (std::size_t index = 1; index < arguments.size(); ++index) {
        const std::string_view argument{arguments[index]};
        if (argument == "--validate") {
            validate = true;
        } else if (argument == "--seconds" && index + 1 < arguments.size()) {
            const std::string value{arguments[++index]};
            float seconds = 0.0F;
            if (std::from_chars(value.c_str(), std::to_address(value.end()), seconds).ec == std::errc{}) {
                lifetime.seconds = seconds;
            }
        }
    }
    try {
        auto registry = std::make_unique<plugins::backend::BackendRegistry>();
        platform::glfw::registerPlatform(*registry);
        render::vulkan::registerRenderer(*registry);

        ecs::App app;
        app.addPlugins(ecs::MinimalPlugins{})
            .addPlugin(plugins::backend::BackendPlugin{std::move(registry), makeSettings(validate)})
            // 980 units/s^2 down is 9.8 m/s^2 at 100 units per meter.
            .addPlugin(PhysicsPlugin{PhysicsSettings{.gravity = {0.0F, 980.0F}, .unitsPerMeter = 100.0F}})
            .addPlugin(debug::PhysicsDebugPlugin{});
        app.insertResource(plugins::backend::ClearColor{.value = {0.05F, 0.05F, 0.08F, 1.0F}});
        app.insertResource(lifetime);
        app.addSystem(ecs::Phase::kStartup, "example.spawnScene", &spawnScene);
        app.addSystem(ecs::Phase::kUpdate, "example.quitWhenDone", &quitWhenDone);
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "PhysicsDebug: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "PhysicsDebug: unknown error\n";
    }
    return 1;
}
