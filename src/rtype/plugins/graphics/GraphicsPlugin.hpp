/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GraphicsPlugin
*/

#pragma once

#include <memory>
#include <span>
#include <string_view>

#include "core/config/Settings.hpp"
#include "plugins/graphics/BackendRegistry.hpp"
#include "plugins/graphics/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::graphics {
/// @brief The window and the renderer for an App: starts the platform and the renderer the configuration names,
/// turns the platform's events into ECS events, and frames the drawing.
///
/// @details The registry comes filled by the executable (the only place that knows GLFW and Vulkan), so this plugin
/// depends on no backend. It adds client systems only, so a dedicated server has no window:
/// - Startup "rtype.graphics.create": creates the Backend (BackendRegistry::createBackend()) into the main-thread
///   resource "rtype.Backend".
/// - PreUpdate "rtype.graphics.events": polls the platform (or waits for an event while the window is minimized),
///   sends every event as a PlatformEvent, in order, and as its typed ECS event (the input ones of InputEvents.hpp,
///   and Closed, Resized, TextEntered), resizes the renderer, and sends AppExit once the window is closed. It runs
///   before the InputPlugin's system.
/// - Render "rtype.graphics.textures": destroys the unloaded textures and loads the queued ones of the TextureAssets
///   resource (updateTextures()), so a texture asked for during this frame's Update is drawn in this frame.
/// - Render "rtype.graphics.beginFrame" and "rtype.graphics.endFrame": clear with the ClearColor resource, and show the
///   frame. In between, "rtype.graphics.camera" shows the world from the entity with a Camera (the highest priority)
///   and a GlobalTransform, through IRenderer::setView() (none: the renderer's default view, window pixels from the
///   top-left corner). Drawing systems run between the two (`.after("rtype.graphics.beginFrame")`,
///   `.before("rtype.graphics.endFrame")`). Both are skipped while the window is minimized. The camera system also
///   records what the frame shows in the VisibleArea resource, for systems that skip what is off screen.
/// - Render "rtype.graphics.sprites", after the camera: draws what the plugins pushed to the SpriteQueue resource
///   before it, sorted by layer (higher on top), then by texture within a layer (fewer batches), and empties the queue.
/// It also inserts the TextureAssets (the textures by AssetId), SpriteQueue and VisibleArea resources. It requires the
/// InputPlugin, as its events system runs before the InputPlugin's, and adds the TransformPlugin, which every game has.
/// It adds no other plugin: the game lists what it draws (SpriteDrawPlugin, TilemapPlugin, TextPlugin, ParticlePlugin,
/// ShaderPlugin), and they know this plugin to run between its systems.
/// @code
/// auto registry = std::make_unique<rtype::plugins::graphics::BackendRegistry>();
/// rtype::platform::glfw::registerPlatform(registry->platforms);
/// rtype::render::vulkan::registerRenderer(registry->renderers);
/// app.addPlugin(rtype::plugins::input::InputPlugin{});
/// app.addPlugin(rtype::plugins::graphics::GraphicsPlugin{std::move(registry), settings});
/// @endcode
class RTYPE_PLUGIN_GRAPHICS_API GraphicsPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.graphics";                     ///< The plugin's name.
    static constexpr std::string_view kCreateSystem = "rtype.graphics.create";      ///< Startup: creates the Backend.
    static constexpr std::string_view kEventsSystem = "rtype.graphics.events";      ///< PreUpdate: sends the events.
    static constexpr std::string_view kTexturesSystem = "rtype.graphics.textures";  ///< Render: loads the textures.
    static constexpr std::string_view kBeginFrameSystem = "rtype.graphics.beginFrame";  ///< Render: starts the frame.
    static constexpr std::string_view kCameraSystem = "rtype.graphics.camera";          ///< Render: applies the camera.
    static constexpr std::string_view kSpritesSystem = "rtype.graphics.sprites";        ///< Render: draws the sprites.
    static constexpr std::string_view kEndFrameSystem = "rtype.graphics.endFrame";      ///< Render: shows the frame.

    /// @param registry The platforms and renderers, registered by the executable.
    /// @param settings The configuration: `platform`, `renderer`, `window` and a section per backend.
    GraphicsPlugin(std::unique_ptr<plugins::graphics::BackendRegistry> registry, core::config::Settings settings);
    ~GraphicsPlugin() override = default;
    GraphicsPlugin(const GraphicsPlugin& other) = delete;
    GraphicsPlugin& operator=(const GraphicsPlugin& other) = delete;
    GraphicsPlugin(GraphicsPlugin&& other) noexcept = default;
    GraphicsPlugin& operator=(GraphicsPlugin&& other) noexcept = default;

    /// @return "rtype.graphics".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The InputPlugin, which reads the events this plugin sends.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Registers the window and input events, inserts the resources and adds the systems. Opens nothing: the
    /// window is created by the Startup system.
    void build(ecs::App& app) override;

  private:
    std::unique_ptr<plugins::graphics::BackendRegistry> _registry;  ///< Handed to the Startup system by build().
    core::config::Settings _settings;                               ///< Handed to the Startup system by build().
};
}  // namespace rtype::plugins::graphics
