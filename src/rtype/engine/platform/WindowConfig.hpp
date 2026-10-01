/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WindowConfig
*/

#pragma once

#include <glm/ext/vector_uint2.hpp>
#include <string>

namespace rtype::engine::platform {

/// @brief How the window is created, passed to IPlatform::init().
struct WindowConfig {
    glm::uvec2 size{1280, 720};  ///< In screen coordinates.
    std::string title = "R-Type";
    bool resizable = true;
    bool fullscreen = false;  ///< On the primary monitor, at its current resolution.
};
}  // namespace rtype::engine::platform
