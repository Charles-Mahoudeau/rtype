/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Particles drawn by Vulkan among the sprites: a fountain between a dark panel (below it) and a
// translucent one (above it), a trail that follows the mouse, and an explosion where the left button is pressed
// (SpawnParticles), and rings on the left animated by a flipbook. Space switches the fountain and the trail between the
// GPU and the CPU simulations, which must look the same; the explosions run where the renderer picks (kAuto: the GPU on
// Vulkan). The particle texture is a soft dot built in code. The view is the window, one world unit per pixel.
// --validation enables the Vulkan validation layer.
//   xmake f --particle=y && xmake run particle [--validation]

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <memory>
#include <numbers>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "core/config/Settings.hpp"
#include "platform/glfw/Registration.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/event/Event.hpp"
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/input/InputEvents.hpp"  // IWYU pragma: keep
#include "plugins/particle/ParticleEffect.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "plugins/particle/ParticleEmitter.hpp"
#include "plugins/particle/ParticleEvents.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextRenderer.hpp"
#include "plugins/transform/Transform.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

using namespace rtype;
using namespace rtype::plugins::particle;
using rtype::core::asset::AssetId;
using rtype::plugins::backend::SpriteRenderer;
using rtype::plugins::backend::graphics::BlendMode;
using rtype::plugins::backend::graphics::Color;
using rtype::plugins::text::FontAssets;
using rtype::plugins::text::TextRenderer;
using rtype::plugins::transform::Transform;

namespace {
constexpr glm::vec2 kWindow{1280.0F, 800.0F};
constexpr Color kClear{0.03F, 0.03F, 0.06F, 1.0F};
constexpr std::uint32_t kDotSize = 32;
constexpr std::uint32_t kRingFrames = 4;
constexpr float kPi = std::numbers::pi_v<float>;
constexpr const char* kFont = "assets/fonts/orbitron.json";

// The ids of what Startup creates, for the Update systems.
struct Assets {
    AssetId explosion;  // The effect spawned on a click.
    AssetId fountain;   // The looping effect between the panels.
    AssetId trail;      // The effect that follows the mouse.
};
RTYPE_ECS_RESOURCE(Assets, "example.Assets");

// Where the mouse is, in window pixels (world units here).
struct Mouse {
    glm::vec2 position{kWindow * 0.5F};
};
RTYPE_ECS_RESOURCE(Mouse, "example.Mouse");

// Where the fountain and the trail are simulated, switched by Space.
struct Mode {
    plugins::backend::graphics::ParticleSimulation simulation = plugins::backend::graphics::ParticleSimulation::kGpu;
};
RTYPE_ECS_RESOURCE(Mode, "example.Mode");

// The emitter that follows the mouse.
struct Trail {};
RTYPE_ECS_COMPONENT(Trail, "example.Trail");

// An emitter respawned when the mode switches: an emitter never changes path while it plays.
struct Switched {};
RTYPE_ECS_COMPONENT(Switched, "example.Switched");

// The text that shows the mode.
struct Hud {};
RTYPE_ECS_COMPONENT(Hud, "example.Hud");

// The text of the HUD for @p simulation.
std::string describe(plugins::backend::graphics::ParticleSimulation simulation) {
    return simulation == plugins::backend::graphics::ParticleSimulation::kGpu ? "GPU simulation (Space: CPU)"
                                                                              : "CPU simulation (Space: GPU)";
}

// Spawns the fountain (layer 1, between the panels) and the trail (layer 3, on top), simulated as @p mode says.
void spawnEmitters(ecs::Commands& commands, const Assets& assets, const Mode& mode, glm::vec2 mouse) {
    commands.spawn()
        .add(Transform{.position = {kWindow.x * 0.5F, kWindow.y * 0.8F}})
        .add(ParticleEmitter{
            .effect = assets.fountain, .layer = 1, .overrideSimulation = true, .simulation = mode.simulation})
        .add(Switched{});
    commands.spawn()
        .add(Transform{.position = mouse})
        .add(ParticleEmitter{
            .effect = assets.trail, .layer = 3, .overrideSimulation = true, .simulation = mode.simulation})
        .add(Trail{})
        .add(Switched{});
}

// A white dot, opaque at its centre and fading to transparent at its edge.
std::vector<std::byte> makeDot() {
    std::vector<std::byte> pixels(static_cast<std::size_t>(kDotSize) * kDotSize * 4);
    const float centre = (static_cast<float>(kDotSize) - 1.0F) * 0.5F;
    for (std::uint32_t y = 0; y < kDotSize; ++y) {
        for (std::uint32_t x = 0; x < kDotSize; ++x) {
            const float distance = std::hypot(static_cast<float>(x) - centre, static_cast<float>(y) - centre) / centre;
            const float alpha = std::clamp(1.0F - distance, 0.0F, 1.0F);
            const std::size_t pixel = ((static_cast<std::size_t>(y) * kDotSize) + x) * 4;
            pixels.at(pixel) = std::byte{255};
            pixels.at(pixel + 1) = std::byte{255};
            pixels.at(pixel + 2) = std::byte{255};
            pixels.at(pixel + 3) = static_cast<std::byte>(alpha * alpha * 255.0F);
        }
    }
    return pixels;
}

// A flipbook of kRingFrames frames side by side: a white ring that grows from frame to frame.
std::vector<std::byte> makeRings() {
    constexpr std::uint32_t kWidth = kDotSize * kRingFrames;
    std::vector<std::byte> pixels(static_cast<std::size_t>(kWidth) * kDotSize * 4);
    const float centre = (static_cast<float>(kDotSize) - 1.0F) * 0.5F;
    for (std::uint32_t frame = 0; frame < kRingFrames; ++frame) {
        const float radius = 4.0F + (static_cast<float>(frame) * 3.5F);
        for (std::uint32_t y = 0; y < kDotSize; ++y) {
            for (std::uint32_t x = 0; x < kDotSize; ++x) {
                const float distance = std::hypot(static_cast<float>(x) - centre, static_cast<float>(y) - centre);
                const float alpha = std::clamp(1.0F - (std::abs(distance - radius) / 2.0F), 0.0F, 1.0F);
                const std::size_t pixel = ((static_cast<std::size_t>(y) * kWidth) + (frame * kDotSize) + x) * 4;
                pixels.at(pixel) = std::byte{255};
                pixels.at(pixel + 1) = std::byte{255};
                pixels.at(pixel + 2) = std::byte{255};
                pixels.at(pixel + 3) = static_cast<std::byte>(alpha * 255.0F);
            }
        }
    }
    return pixels;
}

// Rings rising slowly, growing frame by frame (a flipbook played twice over their life), fading out.
ParticleEffect makeRingEffect(AssetId rings) {
    ParticleEffect effect;
    effect.texture = rings;
    effect.flipbook = {.columns = kRingFrames, .cycles = 2.0F};
    effect.desc.rate = 8.0F;
    effect.desc.lifetime = {.min = 1.6F, .max = 2.0F};
    effect.desc.speed = {.min = 30.0F, .max = 50.0F};
    effect.desc.angle = {.min = (-kPi * 0.5F) - 0.2F, .max = (-kPi * 0.5F) + 0.2F};
    effect.desc.size = {.min = 40.0F, .max = 56.0F};
    effect.desc.color = {0.7F, 0.6F, 1.0F, 1.0F};
    effect.desc.colorOverLife.keys = {{.t = 0.0F, .value = {1.0F, 1.0F, 1.0F, 1.0F}},
                                      {.t = 1.0F, .value = {1.0F, 1.0F, 1.0F, 0.0F}}};
    return effect;
}

// Sparks thrown up, falling back under gravity, orange to red, fading out.
ParticleEffect makeFountain(AssetId dot) {
    ParticleEffect effect;
    effect.texture = dot;
    effect.desc.capacity = 2048;
    effect.desc.rate = 300.0F;
    effect.desc.lifetime = {.min = 1.5F, .max = 2.2F};
    effect.desc.speed = {.min = 300.0F, .max = 420.0F};
    effect.desc.angle = {.min = (-kPi * 0.5F) - 0.3F, .max = (-kPi * 0.5F) + 0.3F};
    effect.desc.size = {.min = 6.0F, .max = 12.0F};
    effect.desc.gravity = {0.0F, 380.0F};
    effect.desc.colorOverLife.keys = {{.t = 0.0F, .value = {1.0F, 0.85F, 0.3F, 1.0F}},
                                      {.t = 0.6F, .value = {1.0F, 0.35F, 0.1F, 0.8F}},
                                      {.t = 1.0F, .value = {0.6F, 0.05F, 0.0F, 0.0F}}};
    return effect;
}

// One burst of sparks in every direction, turned along their velocity and stretched by their speed, slowed by drag,
// shrinking as they fade.
ParticleEffect makeExplosion(AssetId dot) {
    ParticleEffect effect;
    effect.texture = dot;
    effect.desc.loop = false;
    effect.desc.duration = 0.05F;
    effect.desc.bursts.push_back({.count = 200});
    effect.desc.lifetime = {.min = 0.4F, .max = 1.0F};
    effect.desc.speed = {.min = 60.0F, .max = 420.0F};
    effect.desc.size = {.min = 6.0F, .max = 10.0F};
    effect.desc.drag = 3.0F;
    effect.alignment = plugins::backend::graphics::ParticleAlignment::kVelocity;
    effect.stretch = 0.06F;
    effect.desc.colorOverLife.keys = {{.t = 0.0F, .value = {1.0F, 1.0F, 0.8F, 1.0F}},
                                      {.t = 0.3F, .value = {1.0F, 0.6F, 0.2F, 1.0F}},
                                      {.t = 1.0F, .value = {0.5F, 0.1F, 0.1F, 0.0F}}};
    effect.desc.sizeOverLife.keys = {{.t = 0.0F, .value = 1.0F}, {.t = 1.0F, .value = 0.3F}};
    return effect;
}

// Slow cyan dots left behind where the emitter was (kWorld), shrinking to nothing.
ParticleEffect makeTrail(AssetId dot) {
    ParticleEffect effect;
    effect.texture = dot;
    effect.desc.rate = 250.0F;
    effect.desc.lifetime = {.min = 0.4F, .max = 0.7F};
    effect.desc.speed = {.min = 0.0F, .max = 25.0F};
    effect.desc.size = {.min = 10.0F, .max = 14.0F};
    effect.desc.shape = plugins::backend::graphics::CircleShape{.radius = 4.0F};
    effect.desc.color = {0.3F, 0.8F, 1.0F, 1.0F};
    effect.desc.sizeOverLife.keys = {{.t = 0.0F, .value = 1.0F}, {.t = 1.0F, .value = 0.0F}};
    return effect;
}

// Startup: the dot and a white texel, the effects, the two panels around the fountain, the fountain and the trail.
void spawnScene(ecs::ResMut<plugins::backend::TextureAssets> textures, ecs::ResMut<ParticleEffectAssets> effects,
                ecs::ResMut<FontAssets> fonts, ecs::ResMut<Assets> assets, ecs::Res<Mode> mode,
                ecs::Commands& commands) {
    const std::vector<std::byte> dotPixels = makeDot();
    const AssetId dot = textures->loadPixels(
        "example.dot", dotPixels,
        {.size = {kDotSize, kDotSize}, .filter = plugins::backend::graphics::TextureFilter::kLinear});
    const std::vector<std::byte> white(4, std::byte{255});
    const AssetId texel = textures->loadPixels("example.white", white, {.size = {1, 1}});
    const std::vector<std::byte> ringPixels = makeRings();
    const AssetId rings = textures->loadPixels(
        "example.rings", ringPixels,
        {.size = {kDotSize * kRingFrames, kDotSize}, .filter = plugins::backend::graphics::TextureFilter::kLinear});
    // Layer 1, left of the panels: rings animated by a flipbook.
    commands.spawn()
        .add(Transform{.position = {kWindow.x * 0.15F, kWindow.y * 0.8F}})
        .add(ParticleEmitter{.effect = effects->add("example.rings", makeRingEffect(rings)), .layer = 1});
    assets->explosion = effects->add("example.explosion", makeExplosion(dot));
    assets->fountain = effects->add("example.fountain", makeFountain(dot));
    assets->trail = effects->add("example.trail", makeTrail(dot));

    // Layer 0, under the fountain: a dark panel.
    commands.spawn()
        .add(Transform{.position = {kWindow.x * 0.5F, kWindow.y * 0.55F}, .scale = {520.0F, 420.0F}})
        .add(SpriteRenderer{.texture = texel,
                            .source = {{0.0F, 0.0F}, {1.0F, 1.0F}},
                            .origin = {0.5F, 0.5F},
                            .color = {0.12F, 0.14F, 0.25F, 1.0F},
                            .layer = 0});
    // Layer 2, over the fountain's right half: a translucent panel.
    commands.spawn()
        .add(Transform{.position = {kWindow.x * 0.62F, kWindow.y * 0.4F}, .scale = {220.0F, 360.0F}})
        .add(SpriteRenderer{.texture = texel,
                            .source = {{0.0F, 0.0F}, {1.0F, 1.0F}},
                            .origin = {0.5F, 0.5F},
                            .color = {0.2F, 0.8F, 0.4F, 0.5F},
                            .layer = 2});
    spawnEmitters(commands, *assets, *mode, kWindow * 0.5F);
    commands.spawn()
        .add(Transform{.position = {16.0F, 16.0F}})
        .add(TextRenderer{.text = describe(mode->simulation), .font = fonts->load(kFont), .size = 24.0F, .layer = 10})
        .add(Hud{});
}

// Update: on Space, switches the mode, respawns the fountain and the trail in it, and shows it.
void switchSimulation(ecs::EventReader<plugins::backend::event::KeyPressed> keys, ecs::ResMut<Mode> mode,
                      ecs::Res<Assets> assets, ecs::Res<Mouse> mouse,
                      ecs::Query<const Transform, ecs::With<Switched>> switched,
                      ecs::Query<TextRenderer, ecs::With<Hud>> huds, ecs::Commands& commands) {
    bool pressed = false;
    for (const plugins::backend::event::KeyPressed& key : keys) {
        pressed = pressed || (key.key == plugins::backend::input::Key::kSpace && !key.repeat);
    }
    if (!pressed) {
        return;
    }
    mode->simulation = mode->simulation == plugins::backend::graphics::ParticleSimulation::kGpu
                           ? plugins::backend::graphics::ParticleSimulation::kCpu
                           : plugins::backend::graphics::ParticleSimulation::kGpu;
    for (auto [entity, transform] : switched) {
        commands.despawn(entity);
    }
    spawnEmitters(commands, *assets, *mode, mouse->position);
    for (auto [entity, text] : huds) {
        text->text = describe(mode->simulation);
    }
}

// Update: keeps the mouse position, moves the trail to it, and spawns an explosion where the left button is pressed.
void followMouse(ecs::EventReader<plugins::backend::event::MouseMoved> moves,
                 ecs::EventReader<plugins::backend::event::MouseButtonPressed> presses, ecs::ResMut<Mouse> mouse,
                 ecs::Res<Assets> assets, ecs::Query<Transform, ecs::With<Trail>> trails,
                 ecs::EventWriter<SpawnParticles> spawns) {
    for (const plugins::backend::event::MouseMoved& move : moves) {
        mouse->position = move.position;
    }
    for (auto [entity, transform] : trails) {
        transform->position = mouse->position;
    }
    for (const plugins::backend::event::MouseButtonPressed& press : presses) {
        if (press.button == plugins::backend::input::MouseButton::kLeft) {
            spawns.send(SpawnParticles{.effect = assets->explosion, .position = mouse->position, .layer = 3});
        }
    }
}

// The window and the renderer: one world unit per pixel.
core::config::Settings makeSettings(bool validation) {
    core::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "Particles");
    settings.set("window.size", std::vector<double>{kWindow.x, kWindow.y});
    settings.set("vulkan.virtualResolution", std::vector<double>{kWindow.x, kWindow.y});
    if (validation) {
        settings.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
        settings.set("vulkan.debugging", true);
    }
    return settings;
}
}  // namespace

int main(int argc, char** argv) {
    try {
        const std::span<char*> args{argv, static_cast<std::size_t>(argc)};
        const bool validation = args.size() > 1 && std::string_view{args.back()} == "--validation";
        auto registry = std::make_unique<plugins::backend::BackendRegistry>();
        rtype::platform::glfw::registerPlatform(*registry);
        render::vulkan::registerRenderer(*registry);

        ecs::App app;
        app.addPlugins(ecs::MinimalPlugins{})
            .addPlugin(plugins::backend::BackendPlugin{std::move(registry), makeSettings(validation)});
        app.insertResource(plugins::backend::ClearColor{.value = kClear});
        app.insertResource(Assets{});
        app.insertResource(Mouse{});
        app.insertResource(Mode{});
        static_cast<void>(app.getWorld().getComponents().registerType<Trail>());
        static_cast<void>(app.getWorld().getComponents().registerType<Switched>());
        static_cast<void>(app.getWorld().getComponents().registerType<Hud>());
        app.addSystem(ecs::Phase::kStartup, "example.spawnScene", &spawnScene);
        app.addSystem(ecs::Phase::kUpdate, "example.followMouse", &followMouse);
        app.addSystem(ecs::Phase::kUpdate, "example.switchSimulation", &switchSimulation).after("example.followMouse");
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "particle: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "particle: unknown error\n";
    }
    return 1;
}
