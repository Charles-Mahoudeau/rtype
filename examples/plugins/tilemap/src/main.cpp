/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// A side-scrolling level made in Tiled's format (assets/levels/space.tmj): the camera flies right on its own over three
// repeating background layers and two planets, each moving at its own parallax speed, and a long layer of tiles that
// moves with the world. Only the chunks of tiles the camera shows are drawn. At the end of the level, the camera goes
// back to the start.
//   xmake f --tilemap=y && xmake run tilemap

#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "core/config/Settings.hpp"
#include "platform/glfw/Registration.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/Camera.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/GraphicsResources.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/tilemap/SpawnTilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

using namespace rtype;
using namespace rtype::contract::renderer;
using namespace rtype::plugins::graphics;
using namespace rtype::plugins::tilemap;
using rtype::core::asset::AssetId;
using rtype::plugins::transform::Transform;

namespace {
// The view: the size of the background images, shown four times bigger in the window.
constexpr glm::vec2 kView{272.0F, 160.0F};
constexpr float kWindowScale = 4.0F;
constexpr rtype::contract::renderer::Color kClear{0.0F, 0.0F, 0.0F, 1.0F};

constexpr float kCameraSpeed = 48.0F;    // World units per second.
constexpr float kLevelLength = 3200.0F;  // The level's width: 200 tiles of 16 pixels.

// Startup: loads the level, spawns its layers and the camera at its start.
void spawnLevel(ecs::ResMut<TilemapAssets> maps, ecs::Commands& commands) {
    const AssetId level = maps->load("assets/levels/space.tmj");
    const std::vector<ecs::Entity> layers = spawnTilemap(commands, *maps, level);
    std::cout << "assets/levels/space.tmj: " << layers.size() << " layers\n";
    commands.spawn().add(Transform{.position = kView / 2.0F}).add(rtype::plugins::graphics::Camera{});
}

// Update: flies the camera right, back to the start at the end of the level.
void flyCamera(ecs::Res<ecs::Time> time, ecs::Query<Transform, ecs::With<rtype::plugins::graphics::Camera>> cameras) {
    for (auto [entity, transform] : cameras) {
        transform->position.x += kCameraSpeed * time->delta;
        if (transform.get().position.x > kLevelLength - (kView.x / 2.0F)) {
            transform->position.x = kView.x / 2.0F;
        }
    }
}

// The window and the renderer.
core::config::Settings makeSettings() {
    core::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "Tilemap and parallax");
    settings.set("window.size", std::vector<double>{kView.x * kWindowScale, kView.y * kWindowScale});
    settings.set("vulkan.virtualResolution", std::vector<double>{kView.x, kView.y});
    return settings;
}
}  // namespace

int main() {
    try {
        auto registry = std::make_unique<BackendRegistry>();
        rtype::platform::glfw::registerPlatform(registry->platforms);
        render::vulkan::registerRenderer(registry->renderers);

        ecs::App app;
        app.addPlugins(ecs::MinimalPlugins{})
            .addPlugin(rtype::plugins::input::InputPlugin{})
            .addPlugin(GraphicsPlugin{std::move(registry), makeSettings()})
            .addPlugin(rtype::plugins::tilemap::TilemapPlugin{});
        app.insertResource(ClearColor{.value = kClear});
        app.addSystem(ecs::Phase::kStartup, "example.spawnLevel", &spawnLevel);
        app.addSystem(ecs::Phase::kUpdate, "example.flyCamera", &flyCamera);
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "tilemap: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "tilemap: unknown error\n";
    }
    return 1;
}
