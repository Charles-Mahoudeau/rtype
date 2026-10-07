/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Gives the interop interfaces a default visibility, so their RTTI is shared across binaries.
/// @details A renderer in a shared library dynamic_casts a platform created in the executable to an interop
/// interface: both must see the same type_info. Header-only interfaces export nothing, so nothing to do on Windows,
/// where type_info is compared by name.
#ifdef _WIN32
#define RTYPE_INTEROP_API
#else
#define RTYPE_INTEROP_API __attribute__((visibility("default")))
#endif
