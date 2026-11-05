/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ProjectSettings
*/

#pragma once

#include "core/config/Settings.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::engine {
/// @brief The project's settings (`project.luau`), as EnginePlugin loaded them: a resource, so the game's systems read
/// their own keys (`Res<ProjectSettings>`, then `settings->values.getNumber("game.lives", 3)`).
struct ProjectSettings {
    core::config::Settings values;  ///< The flattened `engine.settings` table.
};
RTYPE_ECS_RESOURCE(ProjectSettings, "rtype.ProjectSettings");
}  // namespace rtype::engine
