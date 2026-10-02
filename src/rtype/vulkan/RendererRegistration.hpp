/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RendererRegistration
*/

#pragma once

#include "Export.hpp"
#include "engine/backend/BackendRegistry.hpp"

namespace rtype::vulkan {

/// @brief Registers the renderers of this module: "vulkan" (VulkanRenderer).
///
/// @details Settings of the "vulkan" section, all optional (see VulkanRenderer::Config):
/// - `engineName` (string);
/// - `layers`, `extraExtensions` (lists of strings);
/// - `debugging` (boolean);
/// - `minSeverity` ("verbose", "info", "warning" or "error").
RTYPE_VULKAN_API void registerRenderers(engine::backend::BackendRegistry& registry);

}  // namespace rtype::vulkan
