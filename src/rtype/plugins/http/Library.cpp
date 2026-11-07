/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <string>

#include "core/config/Settings.hpp"
#include "plugins/http/HttpCalls.hpp"
#include "plugins/http/HttpPlugin.hpp"
#include "plugins/http/HttpSettings.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    rtype::plugins::http::addHttpForms(registrar.getPlugins());
    using rtype::plugins::http::HttpPlugin;
    using rtype::plugins::http::HttpSettings;
    registrar.getPlugins().add(std::string{HttpPlugin::kName}, [](const rtype::core::config::Settings& settings) {
        return std::make_unique<HttpPlugin>(HttpSettings::fromSettings(settings.section("http")));
    });
}
