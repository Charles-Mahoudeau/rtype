/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderConfig
*/

#pragma once

#include "plugins/shader/Export.hpp"
#include "rtype/core/config/Settings.hpp"

namespace rtype::plugins::shader {

/// @brief What the ShaderPlugin reads from the project's settings (fromSettings()).
/// @code{.lua}
/// engine.settings.shader = { hotReload = true }
/// @endcode
struct RTYPE_PLUGIN_SHADER_API ShaderConfig {
    bool hotReload{false};  ///< Compiles again the shaders edited under `assets/shaders` of the game.

    /// @brief Reads the section `shader`, whose key `hotReload` is true by default in a debug build and false in a
    /// release one.
    /// @param settings The project's settings, whole.
    /// @throws core::config::exceptions::SettingsException If a key is unknown or has the wrong type.
    [[nodiscard]] static ShaderConfig fromSettings(const core::config::Settings& settings);
};

}  // namespace rtype::plugins::shader
