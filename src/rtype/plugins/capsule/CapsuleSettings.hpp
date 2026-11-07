/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSettings
*/

#pragma once

#include "plugins/capsule/Export.hpp"
#include "rtype/core/config/Settings.hpp"

namespace rtype::plugins::capsule {
/// @brief What the CapsulePlugin reads from the project's settings (fromSettings()).
/// @code{.lua}
/// engine.settings.capsule = { hotReload = true }
/// @endcode
struct RTYPE_PLUGIN_CAPSULE_API CapsuleSettings {
    bool hotReload{
        false};  ///< Reads again the capsule files saved under the game's assets, and reloads what they spawned.

    /// @brief Reads the section `capsule`, whose key `hotReload` is true by default in a debug build and false in a
    /// release one.
    /// @param settings The project's settings, whole.
    /// @throws core::config::exceptions::SettingsException If a key is unknown or has the wrong type.
    [[nodiscard]] static CapsuleSettings fromSettings(const core::config::Settings& settings);
};
}  // namespace rtype::plugins::capsule
