/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendRegistry
*/

#pragma once

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "engine/config/Settings.hpp"
#include "engine/graphics/IRenderer.hpp"
#include "engine/platform/IPlatform.hpp"

namespace rtype::engine::backend {

/// @brief A started platform and renderer, attached to each other.
/// @note The renderer is declared last so it is destroyed first: it may hold resources tied to the window.
struct Backend {
    std::unique_ptr<platform::IPlatform> platform;
    std::unique_ptr<graphics::IRenderer> renderer;
};

/// @brief Platforms and renderers by name, so the configuration picks them instead of the code.
///
/// @details Each module registers its implementations (rtype::platform::registerPlatforms(),
/// rtype::vulkan::registerRenderers()...), then createBackend() starts the pair a config names:
/// @code{.lua}
/// return {
///     platform = "glfw",
///     renderer = "vulkan",
///     window = { title = "R-Type", size = { 1280, 720 } },  -- for every platform
///     vulkan = { debugging = true },                         -- only for the "vulkan" renderer
/// }
/// @endcode
/// Each factory receives the section named after it, and turns it into its own typed configuration. A platform and
/// a renderer that do not fit together are reported by the renderer's init(), which throws.
class BackendRegistry {
  public:
    using PlatformFactory = std::function<std::unique_ptr<platform::IPlatform>(const config::Settings&)>;
    using RendererFactory = std::function<std::unique_ptr<graphics::IRenderer>(const config::Settings&)>;

    BackendRegistry() = default;
    ~BackendRegistry() = default;
    BackendRegistry(const BackendRegistry&) = delete;
    BackendRegistry& operator=(const BackendRegistry&) = delete;
    BackendRegistry(BackendRegistry&&) = delete;
    BackendRegistry& operator=(BackendRegistry&&) = delete;

    /// @brief Registers a platform under a name, replacing any previous one with the same name.
    void addPlatform(std::string name, PlatformFactory factory);

    /// @brief Registers a renderer under a name, replacing any previous one with the same name.
    void addRenderer(std::string name, RendererFactory factory);

    /// @return The registered platform names, sorted.
    [[nodiscard]] std::vector<std::string> getPlatformNames() const;

    /// @return The registered renderer names, sorted.
    [[nodiscard]] std::vector<std::string> getRendererNames() const;

    /// @brief Creates and starts the platform and the renderer the config names, in the engine's order:
    /// IRenderer::prepare(), IPlatform::init() with the `window` section, then IRenderer::init().
    /// @param config The whole configuration: `platform`, `renderer`, `window`, and a section per backend.
    /// @throws exceptions::BackendException If a name is missing or not registered.
    /// @throws exceptions::SettingsException If a setting is unknown or has the wrong type.
    /// @throws exceptions::UnsupportedFeatureException If the renderer does not fit the platform.
    [[nodiscard]] Backend createBackend(const config::Settings& config) const;

  private:
    std::map<std::string, PlatformFactory, std::less<>> _platforms;  ///< Platform factories by name.
    std::map<std::string, RendererFactory, std::less<>> _renderers;  ///< Renderer factories by name.
};
}  // namespace rtype::engine::backend
