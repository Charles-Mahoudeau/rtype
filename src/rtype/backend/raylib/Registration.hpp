/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#pragma once

#include "backend/raylib/Export.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/contract/renderer/RendererRegistry.hpp"

namespace rtype::backend::raylib {

/// @brief Registers the platform "raylib" (RaylibPlatform). It reads the section "raylib" only to check its keys.
RTYPE_BACKEND_RAYLIB_API void registerPlatform(contract::platform::PlatformRegistry& registry);

/// @brief Registers the renderer "raylib" (RaylibRenderer), built from the section "raylib": `virtualResolution`.
RTYPE_BACKEND_RAYLIB_API void registerRenderer(contract::renderer::RendererRegistry& registry);

}  // namespace rtype::backend::raylib
