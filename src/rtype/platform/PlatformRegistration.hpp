/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PlatformRegistration
*/

#pragma once

#include "engine/backend/BackendRegistry.hpp"

namespace rtype::platform {

/// @brief Registers the platforms of this module: "glfw" (GlfwPlatform, no setting of its own).
void registerPlatforms(engine::backend::BackendRegistry& registry);

}  // namespace rtype::platform
