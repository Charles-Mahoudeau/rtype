/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShaderConfig
*/

#pragma once

#include <filesystem>
#include <string_view>

#include "plugins/shader/Export.hpp"
#include "rtype/core/config/Settings.hpp"

namespace rtype::plugins::shader {

/// @brief What the ShaderPlugin reads from the project's settings (fromSettings()).
/// @code{.lua}
/// engine.settings.shader = { hotReload = true }
/// @endcode
struct RTYPE_PLUGIN_SHADER_API ShaderConfig {
    /// @brief The key the Engine fills with the game directory.
    static constexpr std::string_view kGameDirectoryKey = "game.directory";

    std::filesystem::path gameDirectory;  ///< What shader paths are relative to; empty: the working directory.
    bool hotReload{false};                ///< Compiles again the shaders edited under `assets/shaders` of the game.

    /// @brief Reads `game.directory` and the section `shader`, whose key `hotReload` is true by default in a debug
    /// build and false in a release one.
    /// @param settings The project's settings, whole.
    /// @throws core::config::exceptions::SettingsException If a key is unknown or has the wrong type.
    [[nodiscard]] static ShaderConfig fromSettings(const core::config::Settings& settings);
};

}  // namespace rtype::plugins::shader
