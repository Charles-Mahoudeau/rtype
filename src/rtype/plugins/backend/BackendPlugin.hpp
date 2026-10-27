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
#include "plugins/config/Settings.hpp"
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
///   sends every event as a PlatformEvent, in order, and as its typed ECS event (the input ones of InputEvents.hpp,
///   and Closed, Resized, TextEntered), resizes the renderer, and sends AppExit once the window is closed. It runs
///   before the InputPlugin's system.
/// - Render "rtype.backend.textures": destroys the unloaded textures and loads the queued ones of the TextureAssets
///   resource (updateTextures()), so a texture asked for during this frame's Update is drawn in this frame.
/// - Render "rtype.backend.beginFrame" and "rtype.backend.endFrame": clear with the ClearColor resource, and show the
///   frame. In between, "rtype.backend.camera" shows the world from the entity with a Camera (the highest priority)
///   and a GlobalTransform, through IRenderer::setView() (none: the renderer's default view, window pixels from the
///   top-left corner). Drawing systems run between the two (`.after("rtype.backend.beginFrame")`,
///   `.before("rtype.backend.endFrame")`). Both are skipped while the window is minimized. The camera system also
///   records what the frame shows in the VisibleArea resource, for systems that skip what is off screen.
/// - Render "rtype.backend.sprites", after the camera: draws every entity with a GlobalTransform and a SpriteRenderer
///   whose texture is loaded, and the sprites other systems pushed to the SpriteQueue resource before it, sorted by
///   layer (higher on top), then by texture within a layer (fewer batches).
/// It also inserts the TextureAssets (the textures by AssetId), SpriteQueue and VisibleArea resources, and registers
/// the SpriteRenderer component. A window always comes with input, positions, animated sprites and tilemaps: it adds
/// the InputPlugin, the TransformPlugin, the SpritePlugin and the TilemapPlugin (nothing happens if the game added them
/// first). Only the TilemapPlugin knows this plugin, to draw between its camera and its sprites.
/// @code
/// auto registry = std::make_unique<rtype::plugins::backend::BackendRegistry>();
/// rtype::platform::glfw::registerPlatform(*registry);
/// rtype::render::vulkan::registerRenderer(*registry);
/// app.addPlugin(rtype::plugins::backend::BackendPlugin{std::move(registry), settings});
/// @endcode
class BackendPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.backend";                     ///< The plugin's name.
    static constexpr std::string_view kCreateSystem = "rtype.backend.create";      ///< Startup: creates the Backend.
    static constexpr std::string_view kEventsSystem = "rtype.backend.events";      ///< PreUpdate: sends the events.
    static constexpr std::string_view kTexturesSystem = "rtype.backend.textures";  ///< Render: loads the textures.
    static constexpr std::string_view kBeginFrameSystem = "rtype.backend.beginFrame";  ///< Render: starts the frame.
    static constexpr std::string_view kCameraSystem = "rtype.backend.camera";          ///< Render: applies the camera.
    static constexpr std::string_view kSpritesSystem = "rtype.backend.sprites";        ///< Render: draws the sprites.
    static constexpr std::string_view kEndFrameSystem = "rtype.backend.endFrame";      ///< Render: shows the frame.

    /// @param registry The platforms and renderers, registered by the executable.
    /// @param settings The configuration: `platform`, `renderer`, `window` and a section per backend.
    BackendPlugin(std::unique_ptr<plugins::backend::BackendRegistry> registry, plugins::config::Settings settings);
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
    plugins::config::Settings _settings;                           ///< Handed to the Startup system by build().
};
}  // namespace rtype::plugins::backend
