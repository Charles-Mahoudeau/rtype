/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PersistenceSettings
*/

#pragma once

#include <string>

#include "core/config/Settings.hpp"
#include "plugins/persistence/Export.hpp"

namespace rtype::plugins::persistence {

/// @brief The `persistence` section of `project.luau`. Every key is optional.
struct PersistenceSettings {
    std::string directory;        ///< Absolute folder of the saves; empty: `<user data directory>/<folder>`.
    std::string folder{"rtype"};  ///< The game's folder name in the user's data directory.

    /// @throws core::config::exceptions::SettingsException If a key is unknown, has the wrong type or is invalid.
    [[nodiscard]] RTYPE_PLUGIN_PERSISTENCE_API static PersistenceSettings fromSettings(
        const core::config::Settings& settings);
};

}  // namespace rtype::plugins::persistence
