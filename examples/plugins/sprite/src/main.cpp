/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Two animated sprites, from the two ways to describe an atlas:
// - on the left, a sheet with no description (assets/r-typesheet24.gif), cut in code by SpriteAtlas::grid(), its
//   animation added with addAnimation();
// - on the right, a sheet described by a JSON file in Aseprite's format (assets/r-typesheet26.json), which names its
//   frames and its animations (the "left" and "right" frame tags), loaded with AtlasAssets::load().
// Each gets a SpriteAnimator; the sprite plugin, added by the backend plugin, plays it into its SpriteRenderer.
//   xmake f --sprite=y && xmake run sprite

#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>

#include "platform/glfw/Registration.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/config/Settings.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAnimator.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/transform/Transform.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

using namespace rtype;
using namespace rtype::plugins::backend;
using rtype::plugins::asset::AssetId;
using namespace rtype::plugins::sprite;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kWidth = 960.0F;
constexpr float kHeight = 540.0F;
constexpr graphics::Color kClear{0.05F, 0.05F, 0.08F, 1.0F};
constexpr glm::vec2 kScale{4.0F};  // The sheets are small: nearest filtering keeps their pixels sharp.

// Startup: builds the two atlases and spawns a sprite playing each.
void spawnSprites(ecs::ResMut<AtlasAssets> atlases, ecs::Commands& commands) {
    // A sheet with no description: 5 frames of 48x48, 17 pixels apart, the first at (9, 9).
    SpriteAtlas grid = SpriteAtlas::grid("assets/r-typesheet24.gif", {48.0F, 48.0F}, 5, {17.0F, 0.0F}, 0, {9.0F, 9.0F});
    grid.addAnimation("fly", {0, 1, 2, 3, 4}, 0.1F);
    const AssetId fromGrid = atlases->add("example.grid", std::move(grid));

    // A sheet described by its JSON file: the frames, their durations and the "left" and "right" tags come from it.
    const AssetId fromJson = atlases->load("assets/r-typesheet26.json");

    commands.spawn()
        .add(Transform{.position = {kWidth * 0.3F, kHeight / 2.0F}, .scale = kScale})
        .add(SpriteRenderer{})
        .add(SpriteAnimator{.atlas = fromGrid, .animation = animationId("fly")});
    commands.spawn()
        .add(Transform{.position = {kWidth * 0.7F, kHeight / 2.0F}, .scale = kScale})
        .add(SpriteRenderer{})
        .add(SpriteAnimator{.atlas = fromJson, .animation = animationId("left")});
}

// The window and the renderer.
plugins::config::Settings makeSettings() {
    plugins::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "Sprite atlases: grid and JSON");
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
        app.addSystem(ecs::Phase::kStartup, "example.spawnSprites", &spawnSprites);
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "sprite: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "sprite: unknown error\n";
    }
    return 1;
}
