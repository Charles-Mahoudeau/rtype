/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#pragma once

#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/platform/sdl/Export.hpp"

namespace rtype::platform::sdl {

/// @brief Registers the platforms of this module: "sdl" (SdlPlatform, no setting of its own).
RTYPE_PLATFORM_SDL_API void registerPlatform(contract::platform::PlatformRegistry& registry);

}  // namespace rtype::platform::sdl
