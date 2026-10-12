/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Registration
*/

#pragma once

#include "Export.hpp"
#include "engine/backend/BackendRegistry.hpp"

namespace rtype::render::vulkan {

/// @brief Registers the renderers of this module: "vulkan" (VulkanRenderer).
///
/// @details Settings of the "vulkan" section, all optional (see VulkanRenderer::Config):
/// - `engineName` (string);
/// - `layers`, `extraExtensions` (lists of strings);
/// - `debugging` (boolean);
/// - `minSeverity` ("verbose", "info", "warning" or "error");
/// - `preferredDeviceType` ("discrete_gpu", "integrated_gpu", "virtual_gpu", "cpu" or "other", default
///   "discrete_gpu");
/// - `synchronizationValidation` (boolean, default true), `bestPractices` (boolean, default false): extra checks of
///   VK_LAYER_KHRONOS_validation, ignored when it is not in `layers`;
/// - `presentMode` ("fifo", "mailbox" or "immediate", default "fifo"; FIFO when the surface does not support it);
/// - `shaderDirectory` (string, default "shaders"): where the game's SPIR-V shaders are loaded from, relative to the
///   executable's directory unless absolute.
RTYPE_RENDER_VULKAN_API void registerRenderer(engine::backend::BackendRegistry& registry);

}  // namespace rtype::render::vulkan
