/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** main
*/

#include <cstddef>
#include <exception>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "audio/miniaudio/Registration.hpp"
#include "core/config/Settings.hpp"
#include "platform/glfw/Registration.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "plugins/shape/ShapePlugin.hpp"
#include "plugins/sprite/SpriteDrawPlugin.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/Engine.hpp"
#include "rtype/engine/EngineConfig.hpp"

// A two-player duel, the most basic game the engine can run: each player moves a ship in their half of the arena and
// fires lasers; the first laser that touches the other ship wins. Obstacles in the middle stop the lasers, and the
// ships slide along them. The server simulates everything with the physics plugin and replicates Ship, Laser,
// Obstacle and Winner (the physics replicates Collider and RigidBody); the clients send their inputs and draw what
// they receive, animated by the sprite plugin, over a space background whose layers scroll at their own speed (a
// Tiled map), where a planet passes now and then; a ship that is hit explodes. Each player's name is written at the
// top of their half, and the winner in the middle (MSDF text). F3 shows the colliders. One shot in ten misfires: the
// server fires nothing and replicates a Misfire instead. The clients play a looping music, and a sound for every laser
// and misfire, panned where it happened, with a random pitch and volume.
//
// The whole game is in game/: project.luau sets the engine up, and the scripts of game/scripts/ hold the rules, the
// shared ones in arena.luau, the server's in server/ and the clients' in client/. This executable only picks the side
// and registers the client's backends, like r-type_client and r-type_server.
//
// One binary runs both sides: the first argument picks the Engine's mode; the other flags go to game/project.luau,
// which reads --host and --port from `engine.args`.
//
//   rtype_duel server [--port <port>]                  headless server, waits for two clients
//   rtype_duel client [--host <host>] [--port <port>]  a player (default 127.0.0.1:4242)

namespace {

constexpr std::string_view kGame = "examples/games/rtype_duel/game";  ///< From the repository root, xmake's rundir.

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
    app.addPlugin(rtype::plugins::shape::ShapePlugin{});
    app.addPlugin(rtype::plugins::tilemap::TilemapPlugin{});
    app.addPlugin(rtype::plugins::text::TextPlugin{});
    app.addPlugin(rtype::plugins::particle::ParticlePlugin{});
    app.addPlugin(rtype::plugins::audio::AudioPlugin{std::move(audio), settings});
}

/// @return What the Engine runs for @p mode ("server" or "client"), given the flags that follow it.
rtype::engine::EngineConfig makeConfig(const std::string_view mode, std::vector<std::string> args) {
    if (mode == "server") {
        return {.mode = rtype::ecs::RunMode::kServer, .game = kGame, .args = std::move(args)};
    }
    return {
        .mode = rtype::ecs::RunMode::kClient,
        .game = kGame,
        .args = std::move(args),
        .addPlugins = &addClientPlugins,
    };
}

}  // namespace

int main(const int argc, char** argv) {
    try {
        const std::span args{argv, static_cast<std::size_t>(argc)};
        const std::string_view mode = args.size() > 1 ? std::string_view{args[1]} : "client";
        if (mode != "server" && mode != "client") {
            std::cerr
                << "Usage: rtype_duel server [--port <port>] | rtype_duel client [--host <host>] [--port <port>]\n";
            return 1;
        }
        const std::size_t first = args.size() > 1 ? 2 : 1;
        rtype::engine::Engine engine{makeConfig(mode, std::vector<std::string>{args.begin() + first, args.end()})};
        return engine.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
