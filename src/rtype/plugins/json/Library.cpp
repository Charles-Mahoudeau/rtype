/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "core/config/Settings.hpp"
#include "plugins/json/JsonPlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::json::JsonPlugin;
    rtype::plugins::json::addJsonForms(registrar.getPlugins());
    registrar.getPlugins().add(std::string{JsonPlugin::kName},
                               [](const rtype::core::config::Settings&) { return std::make_unique<JsonPlugin>(); });
}
