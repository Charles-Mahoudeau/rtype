/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions exported by the rtype-replication library.
///
/// @details The library is static, linked into the executable: `RTYPE_REPLICATION_STATIC` is defined publicly, so the
/// macro expands to nothing. Built shared instead, `RTYPE_REPLICATION_BUILD` is defined while building it, so the
/// symbols are exported (`__declspec(dllexport)` on Windows, default visibility elsewhere) and imported by its users.
/// Mark every exported class and function with it, and keep no global mutable state, so the library works in both
/// modes.
#ifdef RTYPE_REPLICATION_STATIC
#define RTYPE_REPLICATION_API
#else
#ifdef _WIN32
#ifdef RTYPE_REPLICATION_BUILD
#define RTYPE_REPLICATION_API __declspec(dllexport)
#else
#define RTYPE_REPLICATION_API __declspec(dllimport)
#endif
#else
#define RTYPE_REPLICATION_API __attribute__((visibility("default")))
#endif
#endif
