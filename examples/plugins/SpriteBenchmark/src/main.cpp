/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Measures what animated sprites cost the whole frame: 10 000 of them by default, each playing an animation of the same
// atlas (the six frames of assets/r-typesheet26.gif) from its own start, drifting and bouncing off the window's edges,
// so every frame updates their Transform, advances their animation, sorts them and draws them in one batch.
//
// Each second it prints the frames per second, the mean and the slowest frame time of that second; at the end, the
// mean over the run, after a warm-up second. The issue's target is 60 FPS (16.7 ms) on the team's slowest machine.
// Presentation does not wait for the screen (immediate mode), so the time measured is the engine's, not the display's.
// Where the GPU does not offer it (MoltenVK on macOS), presentation waits for the screen and the frame rate stops at
// the display's (60 or 120 Hz): raise --count until it drops to find the limit. Build in release mode:
//   xmake f -m release --SpriteBenchmark=y && xmake run SpriteBenchmark
//
// Options: --count N sprites (10000), --seconds N to measure (10), --vsync waits for the screen, --validate turns the
// Vulkan validation layer on (much slower).

#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <iterator>
#include <memory>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "platform/glfw/Registration.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAnimator.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
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
using rtype::plugins::asset::AssetId;
using namespace rtype::plugins::sprite;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kWidth = 1280.0F;
constexpr float kHeight = 720.0F;
constexpr graphics::Color kClear{0.02F, 0.02F, 0.04F, 1.0F};

// The atlas: six 63x48 frames, 2 pixels apart, played at 12 frames per second.
constexpr glm::vec2 kFrameSize{63.0F, 48.0F};
constexpr std::uint32_t kFrames = 6;
constexpr float kFrameDuration = 1.0F / 12.0F;

// How fast a sprite drifts, in pixels per second.
constexpr float kMaxSpeed = 120.0F;

// What the command line asks for.
struct Options {
    std::uint32_t count = 10000;
    float seconds = 10.0F;
    bool vsync = false;
    bool validate = false;
};

// A sprite's drift.
struct Drift {
    glm::vec2 velocity{0.0F};
};
RTYPE_ECS_COMPONENT(Drift, "example.Drift");

// The measure: frame times of the current second and of the whole run.
struct Measure {
    std::uint32_t count = 0;
    float seconds = 0.0F;
    float elapsed = 0.0F;
    std::vector<float> second;
    std::vector<float> run;
};
RTYPE_ECS_RESOURCE(Measure, "example.Measure");

// Startup: builds the atlas and spawns the sprites, scattered, each from its own point of the animation.
void spawnSprites(ecs::ResMut<AtlasAssets> atlases, ecs::Res<Measure> measure, ecs::Commands& commands) {
    SpriteAtlas atlas =
        SpriteAtlas::grid("assets/r-typesheet26.gif", kFrameSize, kFrames, {2.0F, 0.0F}, 0, {1.0F, 1.0F});
    atlas.addAnimation("fly", {0, 1, 2, 3, 4, 5}, kFrameDuration);
    const AssetId sheet = atlases->add("benchmark", std::move(atlas));

    // NOLINTNEXTLINE(bugprone-random-generator-seed): the same sprites on every run, so runs compare.
    std::mt19937 random{42};
    std::uniform_real_distribution<float> x{0.0F, kWidth};
    std::uniform_real_distribution<float> y{0.0F, kHeight};
    std::uniform_real_distribution<float> speed{-kMaxSpeed, kMaxSpeed};
    std::uniform_real_distribution<float> start{0.0F, kFrameDuration * static_cast<float>(kFrames)};
    for (std::uint32_t index = 0; index < measure.get().count; ++index) {
        commands.spawn()
            .add(Transform{.position = {x(random), y(random)}, .scale = glm::vec2{0.5F}})
            .add(SpriteRenderer{})
            .add(SpriteAnimator{.atlas = sheet, .animation = animationId("fly"), .time = start(random)})
            .add(Drift{.velocity = {speed(random), speed(random)}});
    }
}

// Update: moves the sprites, bouncing off the edges.
void drift(ecs::Res<ecs::Time> time, ecs::Query<Transform, Drift> sprites) {
    for (auto [entity, transform, sprite] : sprites) {
        glm::vec2& position = transform->position;
        glm::vec2& velocity = sprite->velocity;
        position += velocity * time->delta;
        if (position.x < 0.0F || position.x > kWidth) {
            velocity.x = -velocity.x;
        }
        if (position.y < 0.0F || position.y > kHeight) {
            velocity.y = -velocity.y;
        }
    }
}

// Update: records the frame time, prints each second, and quits after the run with its summary.
void measureFrames(ecs::ResMut<Measure> measure, ecs::Res<ecs::Time> time, ecs::EventWriter<ecs::AppExit> exit) {
    Measure& data = *measure;
    const float milliseconds = time->delta * 1000.0F;
    const bool warmedUp = data.elapsed >= 1.0F;
    data.elapsed += time->delta;
    data.second.push_back(milliseconds);
    if (warmedUp) {
        data.run.push_back(milliseconds);
    }
    if (static_cast<int>(data.elapsed) != static_cast<int>(data.elapsed - time->delta)) {
        float sum = 0.0F;
        for (const float frame : data.second) {
            sum += frame;
        }
        std::cout << std::format("{:3.0f} s  {:5} FPS  mean {:6.2f} ms  slowest {:6.2f} ms{}\n", data.elapsed,
                                 data.second.size(), sum / static_cast<float>(data.second.size()),
                                 *std::ranges::max_element(data.second), warmedUp ? "" : "  (warm-up)");
        data.second.clear();
    }
    if (data.elapsed >= data.seconds + 1.0F && !data.run.empty()) {
        float sum = 0.0F;
        for (const float frame : data.run) {
            sum += frame;
        }
        const float mean = sum / static_cast<float>(data.run.size());
        std::cout << std::format("\n{} animated sprites: mean {:.2f} ms per frame ({:.0f} FPS) over {} frames\n",
                                 data.count, mean, 1000.0F / mean, data.run.size());
        exit.send(ecs::AppExit{.code = 0});
    }
}

// Reads the command line.
Options parseOptions(std::span<char*> arguments) {
    Options options;
    const std::span<char*> given = arguments.empty() ? arguments : arguments.subspan(1);  // Without the program.
    for (auto option = given.begin(); option != given.end(); ++option) {
        const std::string_view argument{*option};
        if (argument == "--vsync") {
            options.vsync = true;
        } else if (argument == "--validate") {
            options.validate = true;
        } else if ((argument == "--count" || argument == "--seconds") && std::next(option) != given.end()) {
            const std::string value{*++option};
            if (argument == "--count") {
                static_cast<void>(std::from_chars(value.c_str(), std::to_address(value.end()), options.count));
            } else {
                static_cast<void>(std::from_chars(value.c_str(), std::to_address(value.end()), options.seconds));
            }
        }
    }
    return options;
}

// The window, the renderer and its presentation mode.
core::config::Settings makeSettings(const Options& options) {
    core::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "Sprite benchmark");
    settings.set("window.size", std::vector<double>{kWidth, kHeight});
    settings.set("vulkan.virtualResolution", std::vector<double>{kWidth, kHeight});
    settings.set("vulkan.presentMode", options.vsync ? "fifo" : "immediate");
    if (options.validate) {
        settings.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
        settings.set("vulkan.debugging", true);
    }
    return settings;
}
}  // namespace

int main(int argc, char** argv) {
    try {
        const Options options = parseOptions({argv, static_cast<std::size_t>(argc)});
        auto registry = std::make_unique<BackendRegistry>();
        rtype::platform::glfw::registerPlatform(*registry);
        render::vulkan::registerRenderer(*registry);

        ecs::App app;
        app.addPlugins(ecs::MinimalPlugins{}).addPlugin(BackendPlugin{std::move(registry), makeSettings(options)});
        app.insertResource(ClearColor{.value = kClear});
        app.insertResource(Measure{.count = options.count, .seconds = options.seconds});
        app.addSystem(ecs::Phase::kStartup, "example.spawnSprites", &spawnSprites);
        app.addSystem(ecs::Phase::kUpdate, "example.drift", &drift);
        app.addSystem(ecs::Phase::kUpdate, "example.measureFrames", &measureFrames);
        std::cout << std::format("{} animated sprites, {} s (after a warm-up second), presentation {}\n", options.count,
                                 options.seconds, options.vsync ? "with vsync" : "immediate");
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "SpriteBenchmark: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "SpriteBenchmark: unknown error\n";
    }
    return 1;
}
