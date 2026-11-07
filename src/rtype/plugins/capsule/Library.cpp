/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "plugins/capsule/CapsulePlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::capsule::CapsulePlugin;
    registrar.getPlugins().add(std::string{CapsulePlugin::kName}, [] { return std::make_unique<CapsulePlugin>(); });
}
