/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Gives the types of `core` that cross binaries a default visibility, so their RTTI is shared.
/// @details `core` is a static library and exports no code: this macro only keeps the type information of its
/// header-only types (the exceptions) visible despite -fvisibility=hidden. Without it, each shared library gets its
/// own `type_info` and an exception thrown in one is not caught by a `catch` in another (macOS). Nothing to do on
/// Windows, where type_info is compared by name.
#ifdef _WIN32
#define RTYPE_CORE_API
#else
#define RTYPE_CORE_API __attribute__((visibility("default")))
#endif

/// @brief Gives a type with no code of its own (an interface, an exception) a default visibility, in any library.
/// @details It exports nothing, as such a type has nothing to export, and a `__declspec(dllimport)` would ask for
/// symbols no library defines. It only keeps its type information visible despite -fvisibility=hidden, for the same
/// reason as RTYPE_CORE_API: a `dynamic_cast` or a `catch` in another binary must recognize it.
#ifdef _WIN32
#define RTYPE_HEADER_ONLY_API
#else
#define RTYPE_HEADER_ONLY_API __attribute__((visibility("default")))
#endif
