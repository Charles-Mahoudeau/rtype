/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Game shaders on sprites and shapes, three ships from one sheet:
// - on the left, the default material;
// - in the middle, a "wave" shader (assets/shaders/wave.glsl): its vertex stage sways the ship with TIME, its
//   fragment stage tints it with a material value and blends in a second texture;
// - on the right, a "flash" shader (assets/shaders/flash.glsl) with an `instance uniform`: each of the two ships sets
//   its own amount (MaterialInstance), so they share one material and one batch;
// - at the bottom, shapes: a bar with the default material, one with "wave", and a circle with "flash" and its own
//   amount. A shape is drawn by a call to the renderer, with the material ShaderHandles created.
// The shaders are files in the engine's GLSL dialect, loaded and compiled at startup by ShaderAssets::load().
//   xmake f --shader=y && xmake run shader

#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "core/config/Settings.hpp"
#include "platform/glfw/Registration.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Shader.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/shader/Material.hpp"
#include "plugins/shader/MaterialAssets.hpp"
#include "plugins/shader/MaterialInstance.hpp"
#include "plugins/shader/ShaderAssets.hpp"
#include "plugins/shader/ShaderHandles.hpp"
#include "plugins/transform/Transform.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

using namespace rtype;
using namespace rtype::plugins::backend;
using namespace rtype::plugins::shader;
using rtype::core::asset::AssetId;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kWidth = 960.0F;
constexpr float kHeight = 540.0F;
constexpr graphics::Color kClear{0.05F, 0.05F, 0.08F, 1.0F};
constexpr glm::vec2 kScale{4.0F};

// Startup: loads and compiles the shaders, builds the materials and spawns the ships.
void spawnShips(ecs::ResMut<TextureAssets> textures, ecs::ResMut<ShaderAssets> shaders,
                ecs::ResMut<MaterialAssets> materials, ecs::Commands& commands) {
    const AssetId sheet = textures->load("assets/r-typesheet24.gif");
    const AssetId overlay = textures->load("assets/r-fx-explosions.gif");
    const AssetId wave = shaders->load("assets/shaders/wave.glsl");
    const AssetId flash = shaders->load("assets/shaders/flash.glsl");
    const AssetId waving = materials->add("example.waving", Material{.shader = wave}.setTexture("overlay", overlay));
    const AssetId flashing = materials->add("example.flashing", Material{.shader = flash});

    const SpriteRenderer ship{.texture = sheet, .source = {{9.0F, 9.0F}, {48.0F, 48.0F}}, .origin = {24.0F, 24.0F}};
    commands.spawn().add(Transform{.position = {kWidth * 0.2F, kHeight / 2.0F}, .scale = kScale}).add(ship);
    SpriteRenderer waved = ship;
    waved.material = waving;
    commands.spawn().add(Transform{.position = {kWidth * 0.5F, kHeight / 2.0F}, .scale = kScale}).add(waved);
    SpriteRenderer flashed = ship;
    flashed.material = flashing;
    const graphics::CompiledShader* compiled = shaders->get(flash);
    if (compiled == nullptr) {
        std::cerr << "shader: " << shaders->getEntry(flash)->error << '\n';
        return;
    }
    commands.spawn()
        .add(Transform{.position = {kWidth * 0.8F, kHeight * 0.3F}, .scale = kScale})
        .add(flashed)
        .add(MaterialInstance::of(*compiled).set(*compiled, "amount", 0.2F));
    commands.spawn()
        .add(Transform{.position = {kWidth * 0.8F, kHeight * 0.7F}, .scale = kScale})
        .add(flashed)
        .add(MaterialInstance::of(*compiled).set(*compiled, "amount", 0.9F));
}

// Render, after the sprites: the shapes, each with the material ShaderHandles created, once it is created.
void drawShapes(ecs::NonSendMut<Backend> backend, ecs::Res<ShaderAssets> shaders, ecs::Res<ShaderHandles> handles) {
    if (backend->renderer == nullptr) {
        return;
    }
    graphics::Shape bar{.kind = graphics::ShapeKind::kRect,
                        .position = {kWidth * 0.2F, kHeight * 0.88F},
                        .size = {220.0F, 44.0F},
                        .cornerRadius = 12.0F,
                        .fillColor = {0.9F, 0.5F, 0.2F, 1.0F},
                        .outlineColor = {1.0F, 1.0F, 1.0F, 1.0F},
                        .outlineThickness = 3.0F};
    backend->renderer->draw(bar);
    if (const ShaderHandles::Material* waving = handles->getMaterial(AssetId::fromPath("example.waving"))) {
        bar.position.x = kWidth * 0.5F;
        bar.material = waving->material;
        bar.instanceValues = waving->instanceDefaults;
        backend->renderer->draw(bar);
    }
    const graphics::CompiledShader* flash = shaders->get(AssetId::fromPath("assets/shaders/flash.glsl"));
    const ShaderHandles::Material* flashing = handles->getMaterial(AssetId::fromPath("example.flashing"));
    if (flash != nullptr && flashing != nullptr) {
        backend->renderer->draw(
            graphics::Shape{.kind = graphics::ShapeKind::kCircle,
                            .position = {kWidth * 0.8F, kHeight * 0.92F},
                            .radius = 28.0F,
                            .fillColor = {0.2F, 0.5F, 0.9F, 1.0F},
                            .material = flashing->material,
                            .instanceValues = MaterialInstance::of(*flash).set(*flash, "amount", 0.5F).values});
    }
}

// The window and the renderer.
core::config::Settings makeSettings() {
    core::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "Game shaders");
    settings.set("window.size", std::vector<double>{kWidth, kHeight});
    settings.set("vulkan.virtualResolution", std::vector<double>{kWidth, kHeight});
    return settings;
}
}  // namespace

int main() {
    try {
        auto registry = std::make_unique<BackendRegistry>();
        rtype::platform::glfw::registerPlatform(*registry);
        render::vulkan::registerRenderer(*registry);

        ecs::App app;
        app.addPlugins(ecs::MinimalPlugins{}).addPlugin(BackendPlugin{std::move(registry), makeSettings()});
        app.insertResource(ClearColor{.value = kClear});
        app.addSystem(ecs::Phase::kStartup, "example.spawnShips", &spawnShips);
        app.addSystem(ecs::Phase::kRender, "example.drawShapes", &drawShapes)
            .after(std::string{BackendPlugin::kSpritesSystem})
            .before(std::string{BackendPlugin::kEndFrameSystem})
            .setSide(ecs::Side::kClient);
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "shader: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "shader: unknown error\n";
    }
    return 1;
}
