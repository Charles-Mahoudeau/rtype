/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "core/config/Settings.hpp"
#include "plugins/shader/ShaderCalls.hpp"
#include "plugins/shader/ShaderPlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    rtype::plugins::shader::addShaderForms(registrar.getPlugins());
    using rtype::plugins::shader::ShaderPlugin;
    registrar.getPlugins().add(std::string{ShaderPlugin::kName}, [](const rtype::core::config::Settings& /*settings*/) {
        return std::make_unique<ShaderPlugin>();
    });
}
