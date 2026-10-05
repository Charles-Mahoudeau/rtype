/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Export
*/

#pragma once

/// @brief The macro that exports symbols from the Vulkan module, for the shared library build.
/// @details The Vulkan module is a shared library, so its symbols must be exported for the engine to use them.
/// Without this, the engine will not find the VulkanRenderer class, and the registration function will not be called.
/// Dynamic cast to IVulkanSurfaceSource will fail, and the engine will throw UnsupportedFeatureException.
#ifdef _WIN32
#ifdef RTYPE_RENDER_VULKAN_BUILD
#define RTYPE_RENDER_VULKAN_API __declspec(dllexport)
#else
#define RTYPE_RENDER_VULKAN_API __declspec(dllimport)
#endif
#else
#define RTYPE_RENDER_VULKAN_API __attribute__((visibility("default")))
#endif
