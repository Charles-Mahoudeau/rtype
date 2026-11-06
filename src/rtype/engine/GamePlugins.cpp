/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GamePlugins
*/

#include "GamePlugins.hpp"

#include <format>
#include <string>
#include <vector>

#include "rtype/core/config/Settings.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/engine/Exceptions.hpp"
#include "rtype/plugins/physics/PhysicsPlugin.hpp"
#include "rtype/plugins/physics/PhysicsSettings.hpp"
#include "rtype/plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "rtype/plugins/physics/debug/PhysicsDebugSettings.hpp"
#include "rtype/plugins/transform/TransformPlugin.hpp"

namespace rtype::engine {
GamePlugins GamePlugins::fromSettings(const core::config::Settings& settings, const ecs::RunMode mode) {
    // Read first, listed or not: a typo in a section fails on every side.
    const plugins::physics::PhysicsSettings physics =
        plugins::physics::PhysicsSettings::fromSettings(settings.section(kPhysics));
    const plugins::physics::debug::PhysicsDebugSettings physicsDebug =
        plugins::physics::debug::PhysicsDebugSettings::fromSettings(settings.section(kPhysicsDebug));

    GamePlugins plugins;
    for (const std::string& name : settings.getStringList(kSettingsKey, {})) {
        if (name == kTransform) {
            plugins.transform = true;
        } else if (name == kPhysics) {
            plugins.physics = physics;
        } else if (name == kPhysicsDebug) {
            if (ecs::runsIn(ecs::Side::kClient, mode)) {
                plugins.physicsDebug = physicsDebug;
            }
        } else {
            throw exceptions::RuntimeError{
                std::format(R"(Invalid setting '{}': unknown plugin '{}' (expected "{}", "{}" or "{}"))", kSettingsKey,
                            name, kTransform, kPhysics, kPhysicsDebug)};
        }
    }
    return plugins;
}

void GamePlugins::addTo(ecs::App& app) const {
    if (transform) {
        app.addPlugin(plugins::transform::TransformPlugin{});
    }
    if (physics) {
        app.addPlugin(plugins::physics::PhysicsPlugin{*physics});
    }
    if (physicsDebug) {
        app.insertResource(*physicsDebug);
        app.addPlugin(plugins::physics::debug::PhysicsDebugPlugin{});
    }
}
}  // namespace rtype::engine
