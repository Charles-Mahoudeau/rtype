/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendRegistry
*/

#pragma once

#include <memory>

#include "core/config/Settings.hpp"
#include "plugins/graphics/Export.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/platform/PlatformRegistry.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/RendererRegistry.hpp"

namespace rtype::plugins::graphics {

/// @brief A started platform and renderer, attached to each other.
/// @note The renderer is declared last so it is destroyed first: it may hold resources tied to the window.
struct Backend {
    std::unique_ptr<contract::platform::IPlatform> platform;
    std::unique_ptr<contract::renderer::IRenderer> renderer;
};

/// @brief The platforms and renderers a program can start, so the configuration picks the pair instead of the code.
///
/// @details Each module registers its implementations (rtype::platform::glfw::registerPlatform(),
/// rtype::render::vulkan::registerRenderer()...), then createBackend() starts the pair a config names:
/// @code{.lua}
/// return {
///     platform = "glfw",
///     renderer = "vulkan",
///     window = { title = "R-Type", size = { 1280, 720 } },  -- for every platform
///     vulkan = { debugging = true },                         -- only for the "vulkan" renderer
/// }
/// @endcode
/// A platform and a renderer that do not fit together are reported by the renderer's init(), which throws.
struct RTYPE_PLUGIN_GRAPHICS_API BackendRegistry {
    contract::platform::PlatformRegistry platforms;  ///< The platforms by name.
    contract::renderer::RendererRegistry renderers;  ///< The renderers by name.

    /// @brief Creates and starts the platform and the renderer the config names, in the engine's order:
    /// IRenderer::setup(), IPlatform::init() with the `window` section, then IRenderer::init().
    /// @param config The whole configuration: `platform`, `renderer`, `window`, and a section per backend.
    /// @throws contract::platform::exceptions::BackendException If a name is missing or not registered.
    /// @throws core::config::exceptions::SettingsException If a setting is unknown or has the wrong type.
    /// @throws contract::platform::exceptions::UnsupportedFeatureException If the renderer does not fit the platform.
    [[nodiscard]] Backend createBackend(const core::config::Settings& config) const;
};

}  // namespace rtype::plugins::graphics
