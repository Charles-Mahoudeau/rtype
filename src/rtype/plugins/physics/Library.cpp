/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "core/config/Settings.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::physics::PhysicsPlugin;
    using rtype::plugins::physics::PhysicsSettings;
    registrar.getPlugins().add(std::string{PhysicsPlugin::kName}, [](const rtype::core::config::Settings& settings) {
        return std::make_unique<PhysicsPlugin>(PhysicsSettings::fromSettings(settings.section("physics")));
    });
}
