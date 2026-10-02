/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowConfig
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <string>

#include "engine/config/Settings.hpp"

namespace rtype::engine::platform {

/// @brief How the window is created, passed to IPlatform::init(). Usually read from the `window` section of the
/// config file (fromSettings()); every field has a default.
struct WindowConfig {
    glm::uvec2 size{1280, 720};  ///< In screen coordinates.
    std::string title = "R-Type";
    bool resizable = true;
    bool fullscreen = false;  ///< On the primary monitor, at its current resolution.

    /// @brief Reads the keys `size` ({width, height}), `title`, `resizable` and `fullscreen`; the missing ones keep
    /// their default.
    /// @throws exceptions::SettingsException If a key is unknown or has the wrong type.
    [[nodiscard]] static WindowConfig fromSettings(const config::Settings& settings);
};
}  // namespace rtype::engine::platform
