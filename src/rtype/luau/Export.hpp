/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the rtype-luau library.
///
/// @details The library is static, linked into the executable: `RTYPE_LUAU_STATIC` is defined publicly, so the macro
/// expands to nothing (a `__declspec(dllimport)` would ask for symbols no DLL defines). Switching the target to a
/// shared library means dropping that define: `RTYPE_LUAU_BUILD` then exports the symbols (`__declspec(dllexport)` on
/// Windows, default visibility elsewhere) and its users import them.
#ifdef RTYPE_LUAU_STATIC
#define RTYPE_LUAU_API
#else
#ifdef _WIN32
#ifdef RTYPE_LUAU_BUILD
#define RTYPE_LUAU_API __declspec(dllexport)
#else
#define RTYPE_LUAU_API __declspec(dllimport)
#endif
#else
#define RTYPE_LUAU_API __attribute__((visibility("default")))
#endif
#endif
