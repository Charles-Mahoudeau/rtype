/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "plugins/ui/UiPlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::plugins::ui::UiPlugin;
    rtype::plugins::ui::addUiForms(registrar.getPlugins());
    registrar.getPlugins().add(std::string{UiPlugin::kName}, [] { return std::make_unique<UiPlugin>(); });
}
