/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorPlugin
*/

#pragma once

#include <span>
#include <string_view>

#include "editor/EditorState.hpp"
#include "editor/Export.hpp"
#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::editor {

/// @brief The editor of a game: the game runs inside the "Game" panel of a window of docked panels, next to the
/// "Entities" and "Inspector" panels, which show and edit the world, and the "Stats" panel, which shows the frame rate
/// and what the renderer drew. It is a library the engine loads, not part of the game: a game gets it by listing
/// "rtype.Editor" under `plugins` in `project.luau`, and a release leaves it out.
///
/// @details The panels are made with Dear ImGui, which lives in this library only. The editor reaches the screen
/// through the renderer contract: the game is drawn into a texture (IRenderer::setSceneSize()) and the panels are drawn
/// as a mesh over the window (IRenderer::drawMesh()). It adds no system: it sets the two hooks the graphics plugin
/// gives a tool, and the graphics plugin, which owns the window, calls them.
/// - The PlatformEventFilter: every event of the window feeds ImGui first, and the game gets the mouse and the keyboard
///   only when its panel has them.
/// - The ToolFrame: once a frame, just before the frame ends, builds the panels and hands their mesh to the renderer.
/// It requires the GraphicsPlugin, and runs on the client side only, as the graphics plugin does.
class RTYPE_EDITOR_API EditorPlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Editor";  ///< The plugin's name.

    /// @param options How the editor is set up: the defaults keep the layout in the user's cache, open no scene, open
    /// files in Visual Studio Code and show the program's output in the Console.
    explicit EditorPlugin(EditorOptions options = {});
    ~EditorPlugin() override = default;
    EditorPlugin(const EditorPlugin& other) = default;
    EditorPlugin& operator=(const EditorPlugin& other) = default;
    EditorPlugin(EditorPlugin&& other) noexcept = default;
    EditorPlugin& operator=(EditorPlugin&& other) noexcept = default;

    /// @return "rtype.Editor".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @return The GraphicsPlugin, which it draws through, and the CapsulePlugin, whose capsules are its scenes.
    [[nodiscard]] std::span<const std::string_view> getDependencies() const noexcept override;

    /// @brief Creates the ImGui context and sets the two hooks of the graphics plugin.
    void build(ecs::App& app) override;

  private:
    EditorOptions _options;  ///< Given at construction.
};

}  // namespace rtype::editor
