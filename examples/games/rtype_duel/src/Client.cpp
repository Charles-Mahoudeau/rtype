/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Client
*/

#include "Client.hpp"

#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Background.hpp"
#include "Components.hpp"
#include "Constants.hpp"
#include "Controls.hpp"
#include "Game.hpp"
#include "Hud.hpp"
#include "Resources.hpp"
#include "Sounds.hpp"
#include "Sprites.hpp"
#include "audio/miniaudio/Registration.hpp"
#include "core/config/Settings.hpp"
#include "engine/scripting/SettingsLoader.hpp"
#include "platform/glfw/Registration.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/replication/client/ClientPlugin.hpp"

using namespace rtype;

namespace rtype_duel {
namespace {

/// @return The configuration of the window, the renderer and the sound.
core::config::Settings makeSettings() {
    engine::scripting::SettingsLoader settingsLoader;
    settingsLoader.setFlag("isClient", true);
    settingsLoader.setFlag("isServer", false);
#ifdef NDEBUG
    settingsLoader.setFlag("isDebug", false);
#else
    settingsLoader.setFlag("isDebug", true);
#endif
    settingsLoader.declareGroups({"window", "audio", "vulkan"});
    settingsLoader.load("game/project.luau");
    return settingsLoader.build();
}

}  // namespace

int runClient(const std::string& host, const std::uint16_t port) {
    const std::optional<network::io::Address> server = network::io::Address::resolve(host, port);
    if (!server) {
        std::cerr << "Error: cannot resolve " << host << '\n';
        return 1;
    }
    // The only place that knows GLFW, Vulkan and miniaudio: the plugins pick them by name from the settings.
    auto registry = std::make_unique<plugins::backend::BackendRegistry>();
    platform::glfw::registerPlatform(*registry);
    render::vulkan::registerRenderer(*registry);
    auto audio = std::make_unique<plugins::audio::AudioRegistry>();
    audio::miniaudio::registerAudio(*audio);

    core::config::Settings settings{makeSettings()};

    ecs::App app{ecs::RunMode::kClient};
    app.addPlugins(ecs::MinimalPlugins{})
        .addPlugin(plugins::backend::BackendPlugin{std::move(registry), settings})
        .addPlugin(plugins::audio::AudioPlugin{std::move(audio), settings});
    registerGame(app);
    app.insertResource(Randomness{});
    app.insertResource(GameSounds{});
    // F3 outlines every collider; hidden at first.
    app.insertResource(plugins::physics::debug::PhysicsDebugSettings{.enabled = false, .drawAll = true});
    app.addPlugin(plugins::physics::debug::PhysicsDebugPlugin{});
    static_cast<void>(app.getWorld().getComponents().registerType<Explosion>());
    app.insertResource(GameAtlases{});
    app.insertResource(GameFonts{});
    static_cast<void>(app.getWorld().getComponents().registerType<BackgroundPlanet>());
    app.insertResource(BackgroundPlanets{});
    app.addPlugin(replication::client::ClientPlugin{*server, network::NetworkSettings{}});
    declareInputs(app);

    app.addSystem(ecs::Phase::kStartup, "game.bindControls", &bindControls).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kStartup, "game.spawnCamera", &spawnCamera).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kStartup, "game.loadSounds", &loadSounds).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.playShots", &playShots).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kStartup, "game.loadAtlases", &loadAtlases).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kStartup, "game.spawnBackground", &spawnBackground).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kStartup, "game.loadPlanets", &loadPlanets).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kStartup, "game.spawnNames", &spawnNames).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.spawnPlanets", &spawnPlanets)
        .after("game.playShots")
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.removePlanets", &removePlanets)
        .after("game.followLasers")
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, "game.sendControls", &sendControls)
        .before(std::string{replication::client::ClientPlugin::kSendInputSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.dressEntities", &dressEntities).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.placeObstacles", &placeObstacles).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, "game.drawObstacles", &drawObstacles)
        .after(std::string{plugins::backend::BackendPlugin::kSpritesSystem})
        .before(std::string{plugins::physics::debug::PhysicsDebugPlugin::kDrawSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.followShips", &followShips).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.followLasers", &followLasers)
        .after("game.followShips")
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.explodeLoser", &explodeLoser).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.removeExplosions", &removeExplosions).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.announceWinner", &announceWinner).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kUpdate, "game.quitOnEscape", &quitOnEscape).setSide(ecs::Side::kClient);
    std::cout << "Connecting to " << host << ':' << port
              << "... WASD or the arrows to move, click to fire, F3 for the colliders.\n";
    return app.run();
}

}  // namespace rtype_duel
