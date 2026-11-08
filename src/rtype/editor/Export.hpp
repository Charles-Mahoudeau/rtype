/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Marks the classes and functions of the rtype-editor library. The library is static, linked into the program:
/// `RTYPE_EDITOR_STATIC` is defined publicly, so the macro expands to nothing. A type with no code of its own, as an
/// interface or an exception, takes RTYPE_HEADER_ONLY_API.
#ifdef RTYPE_EDITOR_STATIC
#define RTYPE_EDITOR_API
#elif defined(_WIN32)
#ifdef RTYPE_EDITOR_BUILD
#define RTYPE_EDITOR_API __declspec(dllexport)
#else
#define RTYPE_EDITOR_API __declspec(dllimport)
#endif
#else
#define RTYPE_EDITOR_API __attribute__((visibility("default")))
#endif
