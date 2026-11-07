/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the rtype-scripting library.
///
/// @details The library is static, linked into the executable: `RTYPE_SCRIPTING_STATIC` is defined publicly, so the
/// macro expands to nothing (a `__declspec(dllimport)` would ask for symbols no DLL defines). Switching the target to a
/// shared library means dropping that define: `RTYPE_SCRIPTING_BUILD` then exports the symbols
/// (`__declspec(dllexport)` on Windows, default visibility elsewhere) and its users import them.
#ifdef RTYPE_SCRIPTING_STATIC
#define RTYPE_SCRIPTING_API
#else
#ifdef _WIN32
#ifdef RTYPE_SCRIPTING_BUILD
#define RTYPE_SCRIPTING_API __declspec(dllexport)
#else
#define RTYPE_SCRIPTING_API __declspec(dllimport)
#endif
#else
#define RTYPE_SCRIPTING_API __attribute__((visibility("default")))
#endif
#endif
