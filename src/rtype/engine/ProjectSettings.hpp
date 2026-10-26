/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ProjectSettings
*/

#pragma once

#include <string>

namespace rtype::engine {
/// @brief The project-wide settings read from the project script (`game/project.luau`).
struct ProjectSettings {
    std::string gameName;     ///< Name of the game, used in the window title.
    std::string gameVersion;  ///< Version of the game, used in the window title.
    std::string platform;     ///< Name of the platform backend in the registry (e.g. "glfw").
    std::string renderer;     ///< Name of the renderer backend in the registry (e.g. "vulkan").
};
}  // namespace rtype::engine
