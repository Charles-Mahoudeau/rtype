/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#pragma once

#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/platform/glfw/Export.hpp"

namespace rtype::platform::glfw {

/// @brief Registers the platforms of this module: "glfw" (GlfwPlatform, no setting of its own).
RTYPE_PLATFORM_GLFW_API void registerPlatform(contract::platform::PlatformRegistry& registry);

}  // namespace rtype::platform::glfw
