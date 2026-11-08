/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PersistenceSettings
*/

#include "plugins/persistence/PersistenceSettings.hpp"

#include <string>

#include "core/config/Settings.hpp"
#include "core/config/exceptions/ConfigExceptions.hpp"

namespace rtype::plugins::persistence {

PersistenceSettings PersistenceSettings::fromSettings(const core::config::Settings& settings) {
    settings.checkKeys({"directory", "folder"}, "persistence");
    PersistenceSettings config;
    config.directory = settings.getString("directory", config.directory);
    config.folder = settings.getString("folder", config.folder);
    if (config.folder.empty() || config.folder.find_first_of("/\\:") != std::string::npos || config.folder == "." ||
        config.folder == "..") {
        throw core::config::exceptions::SettingsException{
            "Setting 'persistence.folder' must be a plain folder name (no separator)"};
    }
    return config;
}

}  // namespace rtype::plugins::persistence
