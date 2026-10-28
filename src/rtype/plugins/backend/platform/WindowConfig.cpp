/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowConfig
*/

#include "WindowConfig.hpp"

#include <string>
#include <vector>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"

namespace rtype::plugins::backend::platform {

WindowConfig WindowConfig::fromSettings(const core::config::Settings& settings) {
    settings.checkKeys({"size", "title", "resizable", "fullscreen"}, "window");
    WindowConfig config;
    const std::vector<double> size =
        settings.getNumberList("size", {static_cast<double>(config.size.x), static_cast<double>(config.size.y)});
    if (size.size() != 2 || size.at(0) < 1.0 || size.at(1) < 1.0) {
        throw core::config::exceptions::SettingsException(
            "Setting 'window.size' must be { width, height }, both positive");
    }
    config.size = {static_cast<unsigned int>(size.at(0)), static_cast<unsigned int>(size.at(1))};
    config.title = settings.getString("title", config.title);
    config.resizable = settings.getBool("resizable", config.resizable);
    config.fullscreen = settings.getBool("fullscreen", config.fullscreen);
    return config;
}

}  // namespace rtype::plugins::backend::platform
