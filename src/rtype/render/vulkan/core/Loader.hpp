/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Loader
*/

#pragma once

#include <vulkan/vulkan_core.h>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::core {
/// @brief The Vulkan loader of the system, opened when the renderer needs it rather than linked.
/// @details The loader (libvulkan.so.1, vulkan-1.dll) comes with the GPU driver, not with the engine, so a release
/// that links it would not start on a machine without one. Opening it here turns that into an error that says what is
/// missing. On macOS the renderer still links the loader, which a release must ship (docs/release/self-contained.md).
///
/// The library stays open until the process ends, as the engine's plugin libraries do: closing it while a driver still
/// has threads is a known way to crash. Opening it again, for another renderer, costs nothing.
class RTYPE_RENDER_VULKAN_API Loader {
  public:
    /// @throws std::runtime_error Naming the files tried, if the system has no Vulkan loader or it has no
    /// vkGetInstanceProcAddr.
    Loader();

    /// @return The entry point of the loader.
    /// @note Hand it to every other Vulkan user (e.g. GLFW) so the whole process shares one loader.
    [[nodiscard]] PFN_vkGetInstanceProcAddr getEntryPoint() const noexcept;

  private:
    PFN_vkGetInstanceProcAddr _entryPoint = nullptr;
};
}  // namespace rtype::render::vulkan::core
