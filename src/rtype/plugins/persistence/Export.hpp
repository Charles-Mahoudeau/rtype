/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the rtype-plugin-persistence library:
/// `RTYPE_PLUGIN_PERSISTENCE_BUILD` is defined while building it.
#ifdef _WIN32
#ifdef RTYPE_PLUGIN_PERSISTENCE_BUILD
#define RTYPE_PLUGIN_PERSISTENCE_API __declspec(dllexport)
#else
#define RTYPE_PLUGIN_PERSISTENCE_API __declspec(dllimport)
#endif
#else
#define RTYPE_PLUGIN_PERSISTENCE_API __attribute__((visibility("default")))
#endif
