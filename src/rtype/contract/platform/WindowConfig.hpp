/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowConfig
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <string>

#include "core/config/Settings.hpp"
#include "rtype/contract/platform/Export.hpp"

namespace rtype::contract::platform {

/// @brief How the window is created, passed to IPlatform::init(). Usually read from the `window` section of the
/// config file (fromSettings()); every field has a default.
struct RTYPE_CONTRACT_PLATFORM_API WindowConfig {
    glm::uvec2 size{1280, 720};  ///< In screen coordinates.
    std::string title = "R-Type";
    bool resizable = true;
    bool fullscreen = false;  ///< On the primary monitor, at its current resolution.
    /// @brief The window system, on Linux: "auto" (Wayland in a Wayland session, X11 otherwise or if Wayland fails),
    /// "x11" or "wayland". Elsewhere there is only one, and this is not read.
    std::string system = "auto";

    /// @brief Reads the keys `size` ({width, height}), `title`, `resizable`, `fullscreen` and `system`; the missing
    /// ones keep their default.
    /// @throws core::config::exceptions::SettingsException If a key is unknown or has the wrong type, or `system` is
    /// not "auto", "x11" or "wayland".
    [[nodiscard]] static WindowConfig fromSettings(const core::config::Settings& settings);
};
}  // namespace rtype::contract::platform
