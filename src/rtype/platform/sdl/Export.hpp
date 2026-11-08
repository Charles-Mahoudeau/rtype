/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the rtype-platform-sdl library: `RTYPE_PLATFORM_SDL_BUILD` is
/// defined while building it, so the symbols are exported (`__declspec(dllexport)` on Windows, default visibility
/// elsewhere) and imported by its users. A type with no code of its own, as an interface or an exception, takes
/// RTYPE_HEADER_ONLY_API.
#ifdef _WIN32
#ifdef RTYPE_PLATFORM_SDL_BUILD
#define RTYPE_PLATFORM_SDL_API __declspec(dllexport)
#else
#define RTYPE_PLATFORM_SDL_API __declspec(dllimport)
#endif
#else
#define RTYPE_PLATFORM_SDL_API __attribute__((visibility("default")))
#endif
