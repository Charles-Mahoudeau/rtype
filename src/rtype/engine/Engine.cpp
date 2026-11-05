/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Engine
*/

#include "Engine.hpp"

#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Exceptions.hpp"
#include "audio/miniaudio/Registration.hpp"
#include "core/config/Settings.hpp"
#include "platform/glfw/Registration.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/AudioRegistry.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "render/vulkan/Registration.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/MinimalPlugins.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/engine/DefaultPlugins.hpp"
#include "rtype/engine/scripting/SettingsLoader.hpp"

namespace rtype::engine {
namespace {
#ifdef NDEBUG
constexpr bool kDebug = false;  ///< Seen by the settings script as `engine.isDebug`.
#else
constexpr bool kDebug = true;  ///< Seen by the settings script as `engine.isDebug`.
#endif

/// @return The project's settings, the script seeing the run mode and the build type as flags.
core::config::Settings loadSettings(ecs::RunMode mode, const std::filesystem::path& project) {
    scripting::SettingsLoader loader;
    loader.setFlag("isClient", mode != ecs::RunMode::kServer);
    loader.setFlag("isServer", mode != ecs::RunMode::kClient);
    loader.setFlag("isDebug", kDebug);
    loader.load(project);
    return loader.build();
}

/// @return The physics settings from the `physics` section: `gravity` ({x, y}) and `unitsPerMeter`.
/// @throws exceptions::RuntimeError If `physics.gravity` does not hold two numbers.
plugins::physics::PhysicsSettings makePhysicsSettings(const core::config::Settings& settings) {
    plugins::physics::PhysicsSettings physics;
    const std::vector<double> gravity =
        settings.getNumberList("physics.gravity", {physics.gravity.x, physics.gravity.y});
    if (gravity.size() != 2) {
        throw exceptions::RuntimeError{
            std::format("Setting 'physics.gravity' needs two numbers {{x, y}}, not {}", gravity.size())};
    }
    physics.gravity = {static_cast<float>(gravity.front()), static_cast<float>(gravity.back())};
    physics.unitsPerMeter =
        static_cast<float>(settings.getNumber("physics.unitsPerMeter", static_cast<double>(physics.unitsPerMeter)));
    return physics;
}
}  // namespace

Engine::Engine(ecs::RunMode mode, const std::filesystem::path& project)
    : _mode{mode},
      _settings{loadSettings(mode, project)},
      _backends{std::make_unique<plugins::backend::BackendRegistry>()},
      _audio{std::make_unique<plugins::audio::AudioRegistry>()},
      _app{mode} {
    platform::glfw::registerPlatform(*_backends);
    render::vulkan::registerRenderer(*_backends);
    audio::miniaudio::registerAudio(*_audio);
    registerPlugins();
    addPlugins();
}

ecs::App& Engine::getApp() noexcept { return _app; }

const ecs::App& Engine::getApp() const noexcept { return _app; }

const core::config::Settings& Engine::getSettings() const noexcept { return _settings; }

int Engine::run() { return _app.run(); }

void Engine::registerPlugins() {
    _plugins.add("backend",
                 [this] { return std::make_unique<plugins::backend::BackendPlugin>(std::move(_backends), _settings); });
    _plugins.add("audio",
                 [this] { return std::make_unique<plugins::audio::AudioPlugin>(std::move(_audio), _settings); });
    _plugins.add("physics",
                 [this] { return std::make_unique<plugins::physics::PhysicsPlugin>(makePhysicsSettings(_settings)); });
    _plugins.add("physics.debug", [] { return std::make_unique<plugins::physics::debug::PhysicsDebugPlugin>(); });
    _plugins.add("particle", [] { return std::make_unique<plugins::particle::ParticlePlugin>(); });
    _plugins.add("text", [] { return std::make_unique<plugins::text::TextPlugin>(); });
    _plugins.add("tilemap", [] { return std::make_unique<plugins::tilemap::TilemapPlugin>(); });
}

void Engine::addPlugins() {
    const std::string key = std::format("plugins.{}", getModeName());
    if (_settings.has(key)) {
        for (const std::string& name : _settings.getStringList(key, {})) {
            _app.addPlugin(_plugins.create(name));
        }
        return;
    }
    if (_mode == ecs::RunMode::kServer) {
        _app.addPlugins(ecs::MinimalPlugins{});  // no window and no sound on a dedicated server
        return;
    }
    _app.addPlugins(DefaultPlugins{std::move(_backends), std::move(_audio), _settings});
}

std::string_view Engine::getModeName() const noexcept {
    switch (_mode) {
        case ecs::RunMode::kServer:
            return "server";
        case ecs::RunMode::kClient:
            return "client";
        case ecs::RunMode::kStandalone:
            return "standalone";
    }
    return "client";
}
}  // namespace rtype::engine
