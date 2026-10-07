/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief Gives the engine's interfaces a default visibility, so their RTTI is shared across binaries.
/// @details engine-core is a static library and exports nothing: this macro does not export code. It only keeps
/// the type information of the header-only interfaces (IPlatform, IRenderer...) visible despite
/// -fvisibility=hidden, so a dynamic_cast done in a shared library (VulkanRenderer finding the platform's
/// IVulkanSurfaceSource) recognizes an object created in the executable. Nothing to do on Windows, where type_info
/// is compared by name.
#ifdef _WIN32
#define RTYPE_ENGINE_API
#else
#define RTYPE_ENGINE_API __attribute__((visibility("default")))
#endif
