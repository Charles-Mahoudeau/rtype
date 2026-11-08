/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "core/config/Settings.hpp"
#include "plugins/persistence/PersistenceCalls.hpp"
#include "plugins/persistence/PersistencePlugin.hpp"
#include "plugins/persistence/PersistenceSettings.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::persistence::PersistencePlugin;
    using rtype::plugins::persistence::PersistenceSettings;
    rtype::plugins::persistence::addPersistenceForms(registrar.getPlugins());
    registrar.getPlugins().add(std::string{PersistencePlugin::kName},
                               [](const rtype::core::config::Settings& settings) {
                                   return std::make_unique<PersistencePlugin>(
                                       PersistenceSettings::fromSettings(settings.section("persistence")));
                               });
}
