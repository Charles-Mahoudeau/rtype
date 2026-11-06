/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

// r-type_client: runs the client side of the game in `game/` (or `--game <directory>`), with the GLFW platform, the
// Vulkan renderer and miniaudio. Every other flag goes to the game's project.luau as `engine.args`.
//   r-type_client [--game <directory>] [--<flag> <value>]...

#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "core/config/Settings.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/sprite/SpriteDrawPlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "rtype/audio/miniaudio/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/platform/glfw/Registration.hpp"
#include "rtype/render/vulkan/Registration.hpp"

namespace {
constexpr int kErrorCode = 84;  ///< What the program returns when it cannot run the game.

/// @brief Adds the plugins of the client side, with the backends the settings can name: the only place that knows
/// GLFW, Vulkan and miniaudio.
void addClientPlugins(rtype::ecs::App& app, const rtype::core::config::Settings& settings) {
    auto backends = std::make_unique<rtype::plugins::graphics::BackendRegistry>();
    rtype::platform::glfw::registerPlatform(backends->platforms);
    rtype::render::vulkan::registerRenderer(backends->renderers);
    auto audio = std::make_unique<rtype::plugins::audio::AudioRegistry>();
    rtype::audio::miniaudio::registerAudio(audio->devices);
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    app.addPlugin(rtype::plugins::graphics::GraphicsPlugin{std::move(backends), settings});
    app.addPlugin(rtype::plugins::shader::ShaderPlugin{});
    app.addPlugin(rtype::plugins::sprite::SpritePlugin{});
    app.addPlugin(rtype::plugins::sprite::SpriteDrawPlugin{});
    app.addPlugin(rtype::plugins::tilemap::TilemapPlugin{});
    app.addPlugin(rtype::plugins::text::TextPlugin{});
    app.addPlugin(rtype::plugins::particle::ParticlePlugin{});
    app.addPlugin(rtype::plugins::audio::AudioPlugin{std::move(audio), settings});
}
}  // namespace

int main(const int argc, char** argv) {
    try {
        const std::span args{argv, static_cast<std::size_t>(argc)};
        rtype::engine::Engine engine{{
            .mode = rtype::ecs::RunMode::kClient,
            .args = std::vector<std::string>{args.begin() + 1, args.end()},
            .addPlugins = &addClientPlugins,
        }};

        return engine.run();
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return kErrorCode;
    }
}
