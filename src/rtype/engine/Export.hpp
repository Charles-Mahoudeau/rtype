/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the rtype-engine library.
///
/// @details The library is static, linked into the executable: `RTYPE_ENGINE_STATIC` is defined publicly, so the
/// macro expands to nothing. Built shared instead, `RTYPE_ENGINE_BUILD` is defined while building it, so the symbols
/// are exported (`__declspec(dllexport)` on Windows, default visibility elsewhere) and imported by its users.
#ifdef RTYPE_ENGINE_STATIC
#define RTYPE_ENGINE_API
#else
#ifdef _WIN32
#ifdef RTYPE_ENGINE_BUILD
#define RTYPE_ENGINE_API __declspec(dllexport)
#else
#define RTYPE_ENGINE_API __declspec(dllimport)
#endif
#else
#define RTYPE_ENGINE_API __attribute__((visibility("default")))
#endif
#endif
