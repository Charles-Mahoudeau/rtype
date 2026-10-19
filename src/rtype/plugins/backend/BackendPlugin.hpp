/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendPlugin
*/

#pragma once

#include <memory>
#include <string_view>

#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/config/Settings.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::backend {
/// @brief The window and the renderer for an App: starts the platform and the renderer the configuration names,
/// turns the platform's events into ECS events, and frames the drawing.
///
/// @details The registry comes filled by the executable (the only place that knows GLFW and Vulkan), so this plugin
/// depends on no backend. It adds client systems only, so a dedicated server has no window:
/// - Startup "rtype.backend.create": creates the Backend (BackendRegistry::createBackend()) into the main-thread
///   resource "rtype.Backend".
/// - PreUpdate "rtype.backend.events": polls the platform (or waits for an event while the window is minimized),
///   sends every event as an ECS event (the input ones of InputEvents.hpp, and Closed, Resized, TextEntered), resizes
///   the renderer, and sends AppExit once the window is closed. It runs before the InputPlugin's system when both
///   plugins are added, whatever their order.
/// - Render "rtype.backend.beginFrame" and "rtype.backend.endFrame": clear with the ClearColor resource, and show the
///   frame. Drawing systems run between the two (`.after("rtype.backend.beginFrame")`,
///   `.before("rtype.backend.endFrame")`). Both are skipped while the window is minimized.
/// It also inserts the TextureAssets resource (the loaded textures) and registers the SpriteRenderer component; drawing
/// the sprites will be a render system between the two frame systems. It does not add the InputPlugin: add it too to
/// read the input as actions.
/// @code
/// auto registry = std::make_unique<rtype::plugins::backend::BackendRegistry>();
/// rtype::platform::glfw::registerPlatform(*registry);
/// rtype::render::vulkan::registerRenderer(*registry);
/// app.addPlugin(rtype::plugins::backend::BackendPlugin{std::move(registry), settings});
/// @endcode
class BackendPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.backend";                 ///< The plugin's name.
    static constexpr std::string_view kCreateSystem = "rtype.backend.create";  ///< Startup: creates the Backend.
    static constexpr std::string_view kEventsSystem = "rtype.backend.events";  ///< PreUpdate: sends the events.
    static constexpr std::string_view kBeginFrameSystem = "rtype.backend.beginFrame";  ///< Render: starts the frame.
    static constexpr std::string_view kEndFrameSystem = "rtype.backend.endFrame";      ///< Render: shows the frame.

    /// @param registry The platforms and renderers, registered by the executable.
    /// @param settings The configuration: `platform`, `renderer`, `window` and a section per backend.
    BackendPlugin(std::unique_ptr<plugins::backend::BackendRegistry> registry,
                  plugins::backend::config::Settings settings);
    ~BackendPlugin() override = default;
    BackendPlugin(const BackendPlugin& other) = delete;
    BackendPlugin& operator=(const BackendPlugin& other) = delete;
    BackendPlugin(BackendPlugin&& other) noexcept = default;
    BackendPlugin& operator=(BackendPlugin&& other) noexcept = default;

    /// @return "rtype.backend".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Registers the window and input events, inserts the resources and adds the systems. Opens nothing: the
    /// window is created by the Startup system.
    void build(ecs::App& app) override;

  private:
    std::unique_ptr<plugins::backend::BackendRegistry> _registry;  ///< Handed to the Startup system by build().
    plugins::backend::config::Settings _settings;                  ///< Handed to the Startup system by build().
};
}  // namespace rtype::plugins::backend
