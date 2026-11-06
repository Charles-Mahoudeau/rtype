/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "plugins/particle/ParticlePlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::particle::ParticlePlugin;
    registrar.getPlugins().add(std::string{ParticlePlugin::kName}, [] { return std::make_unique<ParticlePlugin>(); });
}
