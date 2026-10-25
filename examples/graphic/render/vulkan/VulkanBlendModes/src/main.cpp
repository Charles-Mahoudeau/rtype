/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Opens a GLFW window rendered with Vulkan and draws the same sprites and shapes with each BlendMode, one column per
// mode, over bands of sprite sheet: kAlpha on the left, kAdditive in the middle, kCutout on the right.
//   - a shot, scaled up with linear filtering, slides over the band: its smooth edges show no dark fringe (the
//     textures are premultiplied), and the additive one glows instead of covering;
//   - a ship over a band, tinted at 60% opacity: see-through, glowing, then opaque (a cutout pixel is drawn or not);
//   - two overlapping circles: additive shapes brighten where they meet; a shape has no cutout, so the right column
//     blends them as kAlpha.
// The three shots share one texture: whatever their mode, the renderer draws them in one batch.
// Close the window to quit.
//
// Options: --validate turns the Vulkan validation layer on; --seconds N quits after N seconds.

#include <array>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <initializer_list>
#include <iostream>
#include <iterator>
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
#include "plugins/backend/graphics/AssetId.hpp"
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/transform/Transform.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

using namespace rtype;
using namespace rtype::plugins::backend;
using namespace rtype::plugins::backend::graphics;
using rtype::plugins::transform::Transform;

namespace {
constexpr double kWidth = 800.0;
constexpr double kHeight = 600.0;

// The three columns, from left to right, and the mode each one draws with.
constexpr std::array kColumnX{150.0F, 400.0F, 650.0F};
constexpr std::array kModes{BlendMode::kAlpha, BlendMode::kAdditive, BlendMode::kCutout};

// Rows of the demo.
constexpr float kShipY = 130.0F;
constexpr float kBandY = 300.0F;
constexpr float kCirclesY = 480.0F;

// Frames of the sprite sheets of assets/: top-left corner and size, in pixels.
constexpr glm::vec2 kShotPosition{110.0F, 151.0F};  // bullet.gif: the first shot.
constexpr glm::vec2 kShotSize{78.0F, 16.0F};
constexpr glm::vec2 kShipPosition{9.0F, 9.0F};  // r-typesheet24.gif: the first ship.
constexpr glm::vec2 kShipSize{48.0F, 48.0F};
constexpr glm::vec2 kBandPosition{1.0F, 1.0F};  // r-typesheet26.gif: six frames, one every 65 pixels.
constexpr glm::vec2 kBandSize{63.0F, 48.0F};
constexpr float kBandFrameWidth = 65.0F;
constexpr int kBandFrames = 6;

// Colours of the demo.
constexpr Color kClear{0.05F, 0.05F, 0.08F, 1.0F};
constexpr Color kNone{0.0F, 0.0F, 0.0F, 0.0F};
constexpr Color kSeeThrough{1.0F, 1.0F, 1.0F, 0.6F};
constexpr Color kRed{1.0F, 0.25F, 0.2F, 0.7F};
constexpr Color kBlue{0.2F, 0.45F, 1.0F, 0.7F};

// How far a shot slides each side of its column, and how fast.
constexpr float kSlide = 70.0F;
constexpr float kSlideSpeed = 1.5F;

// A shot sliding over the band, around its column.
struct Shot {
    float columnX = 0.0F;
};
RTYPE_ECS_COMPONENT(Shot, "example.Shot");

// How long the demo runs, if --seconds was given, and how long it has run.
struct Lifetime {
    std::optional<float> seconds;
    float elapsed = 0.0F;
};
RTYPE_ECS_RESOURCE(Lifetime, "example.Lifetime");

// Startup: loads the sprite sheets and spawns the bands, then one ship and one shot per column over them.
void spawnScene(ecs::ResMut<TextureAssets> textures, ecs::Commands& commands) {
    const AssetId shots = textures->load("assets/bullet.gif", TextureDesc{.filter = TextureFilter::kLinear});
    const AssetId ships = textures->load("assets/r-typesheet24.gif");
    const AssetId band = textures->load("assets/r-typesheet26.gif");

    constexpr float kBandScale = 2.0F;
    const float step = kBandSize.x * kBandScale;
    const auto count = static_cast<int>(std::ceil(static_cast<float>(kWidth) / step));
    for (const float y : {kShipY, kBandY}) {
        for (int index = 0; index < count; ++index) {
            const auto frame = static_cast<float>(index % kBandFrames);
            commands.spawn()
                .add(Transform{.position = {step * (static_cast<float>(index) + 0.5F), y},
                               .scale = glm::vec2{kBandScale}})
                .add(SpriteRenderer{
                    .texture = band,
                    .source = {{kBandPosition.x + (frame * kBandFrameWidth), kBandPosition.y}, kBandSize},
                    .origin = kBandSize * 0.5F,
                    .layer = -1});
        }
    }
    for (std::size_t column = 0; column < kColumnX.size(); ++column) {
        commands.spawn()
            .add(Transform{.position = {kColumnX.at(column), kShipY}, .scale = glm::vec2{2.0F}})
            .add(SpriteRenderer{.texture = ships,
                                .source = {kShipPosition, kShipSize},
                                .color = kSeeThrough,
                                .origin = kShipSize * 0.5F,
                                .blend = kModes.at(column)});
        commands.spawn()
            .add(Transform{.position = {kColumnX.at(column), kBandY}, .scale = glm::vec2{3.0F}})
            .add(SpriteRenderer{.texture = shots,
                                .source = {kShotPosition, kShotSize},
                                .origin = kShotSize * 0.5F,
                                .layer = 1,
                                .blend = kModes.at(column)})
            .add(Shot{.columnX = kColumnX.at(column)});
    }
}

// Update: slides the shots back and forth over the band.
void slideShots(ecs::ResMut<Lifetime> lifetime, ecs::Res<ecs::Time> time, ecs::Query<Transform, const Shot> shots,
                ecs::EventWriter<ecs::AppExit> exit) {
    lifetime->elapsed += time->delta;
    const std::optional<float> limit = lifetime.get().seconds;
    if (limit && lifetime.get().elapsed >= *limit) {
        exit.send(ecs::AppExit{.code = 0});
    }
    const float offset = kSlide * std::sin(lifetime.get().elapsed * kSlideSpeed);
    for (auto [entity, transform, shot] : shots) {
        transform->position.x = shot.columnX + offset;
    }
}

// Render, after the sprites: two overlapping circles per column, with the column's mode.
void drawCircles(ecs::NonSendMut<Backend> backend) {
    if (backend->renderer == nullptr) {
        return;
    }
    IRenderer& renderer = *backend->renderer;
    for (std::size_t column = 0; column < kColumnX.size(); ++column) {
        const float x = kColumnX.at(column);
        renderer.draw(Shape{.kind = ShapeKind::kCircle,
                            .position = {x - 25.0F, kCirclesY},
                            .radius = 50.0F,
                            .fillColor = kRed,
                            .outlineColor = kNone,
                            .blend = kModes.at(column)});
        renderer.draw(Shape{.kind = ShapeKind::kCircle,
                            .position = {x + 25.0F, kCirclesY},
                            .radius = 50.0F,
                            .fillColor = kBlue,
                            .outlineColor = kNone,
                            .blend = kModes.at(column)});
    }
}

// The window, the renderer and, when asked, the validation layer.
config::Settings makeSettings(bool validate) {
    config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "Vulkan blend modes");
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
    try {
        bool validate = false;
        Lifetime lifetime;
        const std::span<char*> arguments{argv, static_cast<std::size_t>(argc)};
        const std::span<char*> options = arguments.empty() ? arguments : arguments.subspan(1);  // Without the program.
        for (auto option = options.begin(); option != options.end(); ++option) {
            const std::string_view argument{*option};
            if (argument == "--validate") {
                validate = true;
            } else if (argument == "--seconds" && std::next(option) != options.end()) {
                const std::string value{*++option};
                float seconds = 0.0F;
                if (std::from_chars(value.c_str(), std::to_address(value.end()), seconds).ec == std::errc{}) {
                    lifetime.seconds = seconds;
                }
            }
        }
        auto registry = std::make_unique<BackendRegistry>();
        rtype::platform::glfw::registerPlatform(*registry);
        render::vulkan::registerRenderer(*registry);

        ecs::App app;
        app.addPlugins(ecs::MinimalPlugins{}).addPlugin(BackendPlugin{std::move(registry), makeSettings(validate)});
        app.insertResource(ClearColor{.value = kClear});
        app.insertResource(lifetime);
        app.addSystem(ecs::Phase::kStartup, "example.spawnScene", &spawnScene);
        app.addSystem(ecs::Phase::kUpdate, "example.slideShots", &slideShots);
        app.addSystem(ecs::Phase::kRender, "example.drawCircles", &drawCircles)
            .after(std::string{BackendPlugin::kSpritesSystem})
            .before(std::string{BackendPlugin::kEndFrameSystem});
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "VulkanBlendModes: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "VulkanBlendModes: unknown error\n";
    }
    return 1;
}
