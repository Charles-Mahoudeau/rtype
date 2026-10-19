/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <exception>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "platform/glfw/Registration.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"  // IWYU pragma: keep
#include "plugins/backend/config/Settings.hpp"
#include "plugins/backend/graphics/AssetId.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/input/Control.hpp"
#include "plugins/input/Input.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/AppExit.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"

namespace {
/// @brief Stops the App when Escape is pressed.
void quitOnEscape(rtype::ecs::Res<rtype::plugins::input::Input> input,
                  rtype::ecs::EventWriter<rtype::ecs::AppExit> exit) {
    if (input->read(rtype::plugins::input::Control::key(rtype::plugins::backend::input::Key::kEscape)) > 0.0F) {
        exit.send(rtype::ecs::AppExit{.code = 0});
    }
}

/// @brief Startup: an entity showing the ship sheet; the BackendPlugin loads its texture before the first frame.
void spawnSheet(rtype::ecs::Commands& commands, rtype::ecs::ResMut<rtype::plugins::backend::TextureAssets> textures) {
    commands.spawn().add(rtype::plugins::backend::SpriteRenderer{
        .texture = textures->load("assets/r-typesheet24.gif"),
        .source = rtype::plugins::backend::math::Rect{{0.0F, 0.0F}, {326.0F, 66.0F}},
        .origin = {163.0F, 33.0F},
    });
}

/// @brief Render: draws the SpriteRenderer entities whose texture is loaded, scaled up, at the centre of the view.
/// @todo Replace by the render plugin's system once entities have a Transform.
void drawSprites(rtype::ecs::NonSendMut<rtype::plugins::backend::Backend> backend,
                 rtype::ecs::Res<rtype::plugins::backend::TextureAssets> textures,
                 rtype::ecs::Query<const rtype::plugins::backend::SpriteRenderer> sprites) {
    if (backend->renderer == nullptr) {
        return;
    }
    for (auto [entity, sprite] : sprites) {
        const std::optional<rtype::plugins::backend::graphics::TextureId> texture =
            textures->getTexture(sprite.texture);
        if (!texture) {
            continue;
        }
        backend->renderer->draw(rtype::plugins::backend::graphics::Sprite{.texture = *texture,
                                                                          .source = sprite.source,
                                                                          .position = {400.0F, 300.0F},
                                                                          .scale = {2.0F, 2.0F},
                                                                          .origin = sprite.origin,
                                                                          .color = sprite.color});
    }
}

/// @return The configuration: which platform and renderer to start, and how.
rtype::plugins::backend::config::Settings makeSettings() {
    rtype::plugins::backend::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "R-Type");
    settings.set("window.size", std::vector<double>{800, 600});
    // The world the game is designed for, in world units: the same on a HiDPI screen and in a resized window.
    settings.set("vulkan.virtualResolution", std::vector<double>{800, 600});
#ifndef NDEBUG
    // Debug builds only: the validation layer (with synchronization validation) is slow.
    settings.set("vulkan.layers", std::vector<std::string>{"VK_LAYER_KHRONOS_validation"});
    settings.set("vulkan.debugging", true);
#endif
    return settings;
}
}  // namespace

int main() {
    try {
        // The only place that knows GLFW and Vulkan: the plugins pick them by name from the settings.
        auto registry = std::make_unique<rtype::plugins::backend::BackendRegistry>();
        rtype::platform::glfw::registerPlatform(*registry);
        rtype::render::vulkan::registerRenderer(*registry);

        rtype::ecs::App app;
        app.addPlugins(rtype::ecs::MinimalPlugins{})
            .addPlugin(rtype::plugins::backend::BackendPlugin{std::move(registry), makeSettings()});
        app.addSystem(rtype::ecs::Phase::kUpdate, "rtype.quitOnEscape", &quitOnEscape)
            .setSide(rtype::ecs::Side::kClient);
        app.addSystem(rtype::ecs::Phase::kStartup, "rtype.spawnSheet", &spawnSheet)
            .after(std::string{rtype::plugins::backend::BackendPlugin::kCreateSystem})
            .setSide(rtype::ecs::Side::kClient);
        app.addSystem(rtype::ecs::Phase::kRender, "rtype.drawSprites", &drawSprites)
            .after(std::string{rtype::plugins::backend::BackendPlugin::kCameraSystem})
            .before(std::string{rtype::plugins::backend::BackendPlugin::kEndFrameSystem})
            .setSide(rtype::ecs::Side::kClient);
        return app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
