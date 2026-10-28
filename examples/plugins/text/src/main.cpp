/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// Text from an MSDF font atlas (assets/fonts/orbitron.json, made by msdf-atlas-gen): a line at every size from 12 to
// 200 pixels, all from one 48-pixel atlas, then a text that zooms in and out and turns, sharp at every scale, and
// a centred block of several lines. The view is the window, one world unit per pixel. --validation enables the Vulkan
// validation layer.
//   xmake f --text=y && xmake run text [--validation]

#include <array>
#include <cmath>
#include <cstddef>
#include <exception>
#include <glm/ext/vector_float2.hpp>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "platform/glfw/Registration.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/config/Settings.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextLayout.hpp"
#include "plugins/text/TextRenderer.hpp"
#include "plugins/transform/Transform.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/Time.hpp"

using namespace rtype;
using namespace rtype::plugins::text;
using rtype::plugins::asset::AssetId;
using rtype::plugins::transform::Transform;

namespace {
constexpr glm::vec2 kWindow{1280.0F, 900.0F};
constexpr plugins::backend::graphics::Color kClear{0.05F, 0.05F, 0.09F, 1.0F};
constexpr const char* kFont = "assets/fonts/orbitron.json";
constexpr std::array<float, 8> kSizes{12.0F, 16.0F, 24.0F, 32.0F, 48.0F, 72.0F, 120.0F, 200.0F};
constexpr float kMargin = 24.0F;

// The text that zooms in and out and turns.
struct Zoom {
    float time = 0.0F;  // Seconds since it was spawned.
};
RTYPE_ECS_COMPONENT(Zoom, "example.Zoom");

// Startup: spawns the size of every line written at that size, the zooming text and the centred block.
void spawnTexts(ecs::ResMut<FontAssets> fonts, ecs::Commands& commands) {
    const AssetId font = fonts->load(kFont);
    float y = kMargin;
    for (const float size : kSizes) {
        commands.spawn()
            .add(Transform{.position = {kMargin, y}})
            .add(TextRenderer{.text = std::to_string(static_cast<int>(size)) + " px", .font = font, .size = size});
        y += size * 1.25F;
    }
    commands.spawn()
        .add(Transform{.position = {kWindow.x * 0.78F, kWindow.y * 0.3F}})
        .add(TextRenderer{
            .text = "MSDF", .font = font, .size = 48.0F, .color = {1.0F, 0.6F, 0.2F, 1.0F}, .anchor = {0.5F, 0.5F}})
        .add(Zoom{});
    commands.spawn()
        .add(Transform{.position = {kWindow.x * 0.78F, kWindow.y * 0.6F}})
        .add(TextRenderer{.text = "Player 1 wins!\nCentred lines\nand UTF-8: \xC3\xA9 \xC3\xA0 \xC3\xBC",
                          .font = font,
                          .size = 28.0F,
                          .color = {0.5F, 0.9F, 1.0F, 1.0F},
                          .align = TextAlign::kCenter,
                          .anchor = {0.5F, 0.5F}});
}

// Update: zooms the "MSDF" text from 0.25 to 4 times its size, and turns it.
void zoomText(ecs::Res<ecs::Time> time, ecs::Query<Transform, Zoom> zooms) {
    for (auto [entity, transform, zoom] : zooms) {
        zoom->time += time->delta;
        const float t = zoom.get().time;
        transform->scale = glm::vec2{std::exp2(2.0F * std::sin(t * 0.7F))};
        transform->rotation = 0.3F * std::sin(t * 0.4F);
    }
}

// The window and the renderer: one world unit per pixel.
plugins::config::Settings makeSettings(bool validation) {
    plugins::config::Settings settings;
    settings.set("platform", "glfw");
    settings.set("renderer", "vulkan");
    settings.set("window.title", "MSDF text");
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
        static_cast<void>(app.getWorld().getComponents().registerType<Zoom>());
        app.addSystem(ecs::Phase::kStartup, "example.spawnTexts", &spawnTexts);
        app.addSystem(ecs::Phase::kUpdate, "example.zoomText", &zoomText);
        return app.run();
    } catch (const std::exception& error) {
        std::cerr << "text: " << error.what() << '\n';
    } catch (...) {
        std::cerr << "text: unknown error\n";
    }
    return 1;
}
