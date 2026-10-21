/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Opens a GLFW window rendered with Vulkan and draws every kind of Shape, filled, outlined and turning: rounded
// rectangles, circles, capsules and lines. Every shape of the frame goes through one instanced draw call, on top of the
// sprites (there are none here). Close the window to quit.
//
// Options: --validate turns the Vulkan validation layer on; --seconds N quits after N seconds.

#include <charconv>
#include <cmath>
#include <cstddef>
#include <exception>
#include <glm/ext/vector_float2.hpp>
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
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

using namespace rtype;
using namespace rtype::plugins::backend::graphics;

namespace {
constexpr double kWidth = 800.0;
constexpr double kHeight = 600.0;

// Colours of the demo.
constexpr Color kClear{0.06F, 0.06F, 0.1F, 1.0F};
constexpr Color kNone{0.0F, 0.0F, 0.0F, 0.0F};
constexpr Color kWhite{1.0F, 1.0F, 1.0F, 1.0F};
constexpr Color kGreen{0.2F, 0.9F, 0.4F, 1.0F};
constexpr Color kOrange{1.0F, 0.6F, 0.1F, 1.0F};
constexpr Color kBlue{0.3F, 0.5F, 1.0F, 0.85F};

// How long the demo runs, if --seconds was given, and how long it has run.
struct Lifetime {
    std::optional<float> seconds;
    float elapsed = 0.0F;
};
RTYPE_ECS_RESOURCE(Lifetime, "example.Lifetime");

// Render, after the sprites: draws the demo's shapes, turning with time.
void drawShapes(ecs::NonSendMut<plugins::backend::Backend> backend, ecs::Res<ecs::Time> time,
                ecs::ResMut<Lifetime> lifetime, ecs::EventWriter<ecs::AppExit> exit) {
    lifetime->elapsed += time->delta;
    const std::optional<float> limit = lifetime.get().seconds;
    if (limit && lifetime.get().elapsed >= *limit) {
        exit.send(ecs::AppExit{.code = 0});
    }
    if (backend->renderer == nullptr) {
        return;
    }
    IRenderer& renderer = *backend->renderer;
    const float angle = lifetime->elapsed;

    // Top row: filled rectangles, sharp and rounded, the last one turning.
    renderer.draw(
        Shape{.kind = ShapeKind::kRect, .position = {120.0F, 120.0F}, .size = {140.0F, 80.0F}, .fillColor = kBlue});
    renderer.draw(Shape{.kind = ShapeKind::kRect,
                        .position = {320.0F, 120.0F},
                        .size = {140.0F, 80.0F},
                        .cornerRadius = 20.0F,
                        .fillColor = kOrange,
                        .outlineColor = kWhite,
                        .outlineThickness = 3.0F});
    renderer.draw(Shape{.kind = ShapeKind::kRect,
                        .position = {540.0F, 120.0F},
                        .size = {140.0F, 80.0F},
                        .cornerRadius = 8.0F,
                        .rotation = angle,
                        .fillColor = kNone,
                        .outlineColor = kGreen,
                        .outlineThickness = 2.0F});

    // Middle row: circles, filled, outlined, and outline only (a hitbox, as the physics debug view draws them).
    renderer.draw(Shape{.kind = ShapeKind::kCircle, .position = {120.0F, 300.0F}, .radius = 50.0F, .fillColor = kBlue});
    renderer.draw(Shape{.kind = ShapeKind::kCircle,
                        .position = {320.0F, 300.0F},
                        .radius = 50.0F,
                        .fillColor = kOrange,
                        .outlineColor = kWhite,
                        .outlineThickness = 4.0F});
    renderer.draw(Shape{.kind = ShapeKind::kCircle,
                        .position = {540.0F, 300.0F},
                        .radius = 50.0F,
                        .fillColor = kNone,
                        .outlineColor = kGreen,
                        .outlineThickness = 1.5F});

    // Bottom row: capsules, the last one turning, and a velocity-like arrow made of lines.
    renderer.draw(Shape{.kind = ShapeKind::kCapsule,
                        .position = {120.0F, 480.0F},
                        .radius = 25.0F,
                        .halfHeight = 40.0F,
                        .fillColor = kBlue,
                        .outlineColor = kWhite,
                        .outlineThickness = 2.0F});
    renderer.draw(Shape{.kind = ShapeKind::kCapsule,
                        .position = {320.0F, 480.0F},
                        .radius = 20.0F,
                        .halfHeight = 40.0F,
                        .rotation = angle,
                        .fillColor = kNone,
                        .outlineColor = kGreen,
                        .outlineThickness = 2.0F});
    const glm::vec2 from{480.0F, 480.0F};
    const glm::vec2 to = from + glm::vec2{std::cos(angle), std::sin(angle)} * 120.0F;
    const glm::vec2 direction = (to - from) / 120.0F;
    const glm::vec2 side{-direction.y, direction.x};
    renderer.draw(Shape{.kind = ShapeKind::kLine, .position = from, .end = to, .radius = 2.0F, .fillColor = kGreen});
    renderer.draw(Shape{.kind = ShapeKind::kLine,
                        .position = to,
                        .end = to - (direction - side) * 18.0F,
                        .radius = 2.0F,
                        .fillColor = kGreen});
    renderer.draw(Shape{.kind = ShapeKind::kLine,
                        .position = to,
                        .end = to - (direction + side) * 18.0F,
                        .radius = 2.0F,
                        .fillColor = kGreen});
}

// The window, the renderer and, when asked, the validation layer.
plugins::backend::config::Settings makeSettings(bool validate) {
    plugins::backend::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "Vulkan shapes");
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
            .addPlugin(plugins::backend::BackendPlugin{std::move(registry), makeSettings(validate)});
        app.insertResource(plugins::backend::ClearColor{.value = kClear});
        app.insertResource(lifetime);
        app.addSystem(ecs::Phase::kRender, "example.drawShapes", &drawShapes)
            .after(std::string{plugins::backend::BackendPlugin::kSpritesSystem})
            .before(std::string{plugins::backend::BackendPlugin::kEndFrameSystem});
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "VulkanShapes: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "VulkanShapes: unknown error\n";
    }
    return 1;
}
