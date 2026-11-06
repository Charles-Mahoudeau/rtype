/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "plugins/text/TextCalls.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    rtype::plugins::text::addTextForms(registrar.getPlugins());
    using rtype::plugins::text::TextPlugin;
    registrar.getPlugins().add(std::string{TextPlugin::kName}, [] { return std::make_unique<TextPlugin>(); });
}
