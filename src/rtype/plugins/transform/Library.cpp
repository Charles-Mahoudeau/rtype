/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::transform::TransformPlugin;
    registrar.getPlugins().add(std::string{TransformPlugin::kName}, [] { return std::make_unique<TransformPlugin>(); });
}
