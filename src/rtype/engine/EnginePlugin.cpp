/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EnginePlugin
*/

#include "EnginePlugin.hpp"

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
#include "rtype/engine/PluginRegistry.hpp"
#include "rtype/engine/ProjectSettings.hpp"
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

/// @return The run mode's name in the settings keys ("client", "server", "standalone").
std::string_view modeName(ecs::RunMode mode) noexcept {
    switch (mode) {
        case ecs::RunMode::kServer:
            return "server";
        case ecs::RunMode::kClient:
            return "client";
        case ecs::RunMode::kStandalone:
            return "standalone";
    }
    return "client";
}

/// @return The plugins a project can list by name. The backend and audio factories move their registry into the
/// plugin: listing "backend" twice is harmless, the App skips the second plugin by name before building it.
PluginRegistry makePluginRegistry(std::unique_ptr<plugins::backend::BackendRegistry>& backends,
                                  std::unique_ptr<plugins::audio::AudioRegistry>& audio,
                                  const core::config::Settings& settings) {
    PluginRegistry registry;
    registry.add("backend", [&backends, &settings] {
        return std::make_unique<plugins::backend::BackendPlugin>(std::move(backends), settings);
    });
    registry.add("audio", [&audio, &settings] {
        return std::make_unique<plugins::audio::AudioPlugin>(std::move(audio), settings);
    });
    registry.add("physics", [&settings] {
        return std::make_unique<plugins::physics::PhysicsPlugin>(makePhysicsSettings(settings));
    });
    registry.add("physics.debug", [] { return std::make_unique<plugins::physics::debug::PhysicsDebugPlugin>(); });
    registry.add("particle", [] { return std::make_unique<plugins::particle::ParticlePlugin>(); });
    registry.add("text", [] { return std::make_unique<plugins::text::TextPlugin>(); });
    registry.add("tilemap", [] { return std::make_unique<plugins::tilemap::TilemapPlugin>(); });
    return registry;
}
}  // namespace

EnginePlugin::EnginePlugin(std::filesystem::path project) : _project{std::move(project)} {}

const std::filesystem::path& EnginePlugin::getProject() const noexcept { return _project; }

std::string_view EnginePlugin::getName() const noexcept { return kName; }

void EnginePlugin::build(ecs::App& app) {
    const ecs::RunMode mode = app.getSchedules().getRunMode();
    const core::config::Settings settings = loadSettings(mode, _project);
    app.insertResource(ProjectSettings{.values = settings});

    // The only place, with the examples, that knows GLFW, Vulkan and miniaudio: the plugins pick them by name.
    auto backends = std::make_unique<plugins::backend::BackendRegistry>();
    platform::glfw::registerPlatform(*backends);
    render::vulkan::registerRenderer(*backends);
    auto audio = std::make_unique<plugins::audio::AudioRegistry>();
    audio::miniaudio::registerAudio(*audio);

    const std::string key = std::format("plugins.{}", modeName(mode));
    if (settings.has(key)) {
        const PluginRegistry registry = makePluginRegistry(backends, audio, settings);
        for (const std::string& name : settings.getStringList(key, {})) {
            app.addPlugin(registry.create(name));
        }
        return;
    }
    if (mode == ecs::RunMode::kServer) {
        app.addPlugins(ecs::MinimalPlugins{});  // no window and no sound on a dedicated server
        return;
    }
    app.addPlugins(DefaultPlugins{std::move(backends), std::move(audio), settings});
}
}  // namespace rtype::engine
