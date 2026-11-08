/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Panels
*/

#pragma once

#include <string>

#include "editor/EditorState.hpp"
#include "editor/Export.hpp"
#include "editor/server/ServerSupervisor.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"

/// @brief The panels of the editor: each one draws its own ImGui window, once a frame, if its flag in the EditorState
/// says it is open.
namespace rtype::editor::panels {

/// @brief The main menu bar: File (Save, Quit), View (the panels, the layout), the Play / Pause / Step / Stop
/// buttons, what is being edited and the last message.
RTYPE_EDITOR_API void drawToolbar(EditorState& state, contract::platform::IPlatform& platform);

/// @brief The Game panel: shows the scene the renderer draws into a texture, as big as the panel, and tells the input
/// bridge where it is so the mouse and the keyboard reach the game only when they should.
RTYPE_EDITOR_API void drawGamePanel(EditorState& state, contract::renderer::IRenderer& renderer);

/// @brief The Hierarchy panel: while editing a scene, the tree of its nodes, to select, add, delete and move them (drag
/// one onto another, or drop a capsule file to place an instance of it); while playing, the entities of the world,
/// by their parents, named by their Name.
RTYPE_EDITOR_API void drawHierarchyPanel(EditorState& state);

/// @brief The Inspector panel: the components of the selected node of the scene, or of the selected entity while
/// playing, with their fields editable in place, and components to add and remove.
RTYPE_EDITOR_API void drawInspectorPanel(EditorState& state);

/// @brief The Assets panel: the files of the game's assets, to open a scene, make new scenes, actors and folders,
/// rename and delete, and drag a file onto a node or a field.
RTYPE_EDITOR_API void drawAssetsPanel(EditorState& state);

/// @brief Opens the capsule file @p path (in the assets) as the scene being edited, reading it first if it is not
/// known yet, and shows it in the world. Refused while playing; what went wrong goes to the status line.
RTYPE_EDITOR_API void openScene(EditorState& state, const std::string& path);

/// @brief The Particles panel: the particle effects of the game, edited in place (the emitters playing one restart
/// with it, in the editor's view even while editing), a preview at the middle of the view, and "Copy as Luau", the
/// `engine.assets.particle` table of the effect, to paste in the script that makes it.
RTYPE_EDITOR_API void drawParticlesPanel(EditorState& state);

/// @brief The Prefab window: the capsule opened alone (openPrefab()), drawn from the renderer's preview, in the middle,
/// next to the Game panel, which goes on showing the game. The wheel zooms. Hidden while no capsule is open alone.
RTYPE_EDITOR_API void drawPrefabPanel(EditorState& state, contract::renderer::IRenderer& renderer);

/// @brief Opens the capsule file @p path alone (prefab mode), in the Prefab window: the Hierarchy and the Inspector
/// edit it until closePrefab(). Refused while playing.
RTYPE_EDITOR_API void openPrefab(EditorState& state, const std::string& path);

/// @brief Leaves prefab mode: the capsule shown alone goes, and the scene is edited again.
RTYPE_EDITOR_API void closePrefab(EditorState& state);

/// @brief The Console panel: the program's output (the scripts' `print`, the engine's and the renderer's messages),
/// filtered by kind and by text, errors and warnings in colour.
RTYPE_EDITOR_API void drawConsolePanel(EditorState& state);

/// @brief The Server panel: runs the game's server as a child process (start, stop, restart, its log), shows its status
/// and its clients through the admin channel (kick), sets the network simulation, inspects and edits the server's world
/// (entities, components, spawn, despawn, pause, step), and launches detached clients. It also keeps watching the
/// server and the clients while closed.
RTYPE_EDITOR_API void drawServerPanel(EditorState& state);

/// @return How the Server panel starts the server now: its game folder, port and flags as typed. Play online starts the
/// server the same way.
[[nodiscard]] RTYPE_EDITOR_API server::ServerOptions serverOptionsOf(const EditorState& state);

/// @brief The Stats panel: the frame rate, the plot of the last frame times, and what the renderer drew.
RTYPE_EDITOR_API void drawStatsPanel(EditorState& state, const contract::renderer::IRenderer& renderer);

/// @brief The Plugins panel: the plugins added to the App in the order they were recorded, each with its kind (engine
/// or game), the plugins it requires and those that require it, a filter, and the plugins registered but not added.
RTYPE_EDITOR_API void drawPluginsPanel(EditorState& state);

}  // namespace rtype::editor::panels
