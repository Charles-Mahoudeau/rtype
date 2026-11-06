/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "plugins/tilemap/TilemapPlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::tilemap::TilemapPlugin;
    registrar.getPlugins().add(std::string{TilemapPlugin::kName}, [] { return std::make_unique<TilemapPlugin>(); });
}
