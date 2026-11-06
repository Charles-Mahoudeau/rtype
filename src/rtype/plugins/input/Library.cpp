/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "plugins/input/InputCalls.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    rtype::plugins::input::addInputForms(registrar.getPlugins());
    using rtype::plugins::input::InputPlugin;
    registrar.getPlugins().add(std::string{InputPlugin::kName}, [] { return std::make_unique<InputPlugin>(); });
}
