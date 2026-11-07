/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "core/config/Settings.hpp"
#include "plugins/capsule/CapsuleCalls.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    rtype::plugins::capsule::addCapsuleForms(registrar.getPlugins());
    using rtype::plugins::capsule::CapsulePlugin;
    registrar.getPlugins().add(
        std::string{CapsulePlugin::kName},
        [](const rtype::core::config::Settings& /*settings*/) { return std::make_unique<CapsulePlugin>(); });
}
