/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "core/config/Settings.hpp"
#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::physics::debug::PhysicsDebugPlugin;
    using rtype::plugins::physics::debug::PhysicsDebugSettings;
    registrar.getPlugins().add(std::string{PhysicsDebugPlugin::kName},
                               [](const rtype::core::config::Settings& settings) {
                                   return std::make_unique<PhysicsDebugPlugin>(
                                       PhysicsDebugSettings::fromSettings(settings.section("physicsDebug")));
                               });
}
