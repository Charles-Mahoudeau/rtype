/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#pragma once

#include "engine/backend/BackendRegistry.hpp"

namespace rtype::platform::glfw {

/// @brief Registers the platforms of this module: "glfw" (GlfwPlatform, no setting of its own).
void registerPlatform(engine::backend::BackendRegistry& registry);

}  // namespace rtype::platform::glfw
