/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the rtype-plugin-capsule library:
/// `RTYPE_PLUGIN_CAPSULE_BUILD` is defined while building it, so the symbols are exported (`__declspec(dllexport)` on
/// Windows, default visibility elsewhere) and imported by its users.
#ifdef _WIN32
#ifdef RTYPE_PLUGIN_CAPSULE_BUILD
#define RTYPE_PLUGIN_CAPSULE_API __declspec(dllexport)
#else
#define RTYPE_PLUGIN_CAPSULE_API __declspec(dllimport)
#endif
#else
#define RTYPE_PLUGIN_CAPSULE_API __attribute__((visibility("default")))
#endif
