/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "plugins/shape/ShapePlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::shape::ShapePlugin;
    registrar.getPlugins().add(std::string{ShapePlugin::kName}, [] { return std::make_unique<ShapePlugin>(); });
}
