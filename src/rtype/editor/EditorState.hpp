/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorState
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "editor/EditorSession.hpp"
#include "editor/Export.hpp"
#include "editor/console/ConsoleLog.hpp"
#include "editor/gui/GuiContext.hpp"
#include "editor/gui/GuiInput.hpp"
#include "editor/play/PlaySession.hpp"
#include "editor/process/ClientLauncher.hpp"
#include "editor/scene/Isolation.hpp"
#include "editor/server/ServerPanelState.hpp"
#include "editor/server/ServerSupervisor.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::ecs {
class App;
class World;
}  // namespace rtype::ecs

namespace rtype::editor {

/// @brief What the Assets panel is asking for, through its popup.
enum class AssetAction : std::uint8_t {
    kNone,       ///< Nothing.
    kNewScene,   ///< A new `.scene` in the target folder.
    kNewActor,   ///< A new `.actor` in the target folder.
    kNewFolder,  ///< A new folder in the target folder.
    kRename,     ///< A new name for the target file or folder.
    kDelete,     ///< Removing the target file or folder.
};

/// @brief How the editor is set up: from the `editor` settings, or by a test.
struct EditorOptions {
    std::optional<std::filesystem::path> layoutFile;  ///< Where the layout is kept; empty: not kept; none: the cache.
    std::string scene;                                ///< The scene opened at start (`editor.scene`); empty: none.
    std::string
        textEditor;  ///< The command that opens a file (`editor.textEditor`); empty: shell::defaultTextEditor().
    bool captureOutput{true};  ///< Whether the Console shows the program's output (OutputCapture).
    /// What survives from one App to the next (the child server, the Console, Play online); null: the plugin makes
    /// its own, which lives and dies with its App.
    std::shared_ptr<EditorSession> session;
};

/// @brief Everything the editor keeps between two frames: the Dear ImGui context, the bridge that feeds it, the play
/// session, the scene being edited, what the panels show and what they remember. The plugin's two hooks (the event
/// filter and the tool frame) share it, and it goes with them when the App is destroyed.
struct RTYPE_EDITOR_API EditorState {
    static constexpr std::size_t kFrameHistory = 120;  ///< How many frame times the Stats panel plots.

    /// @param app The App the editor runs in; it must outlive the state.
    /// @param layoutFile Where the layout of the panels is kept; empty: it is not.
    /// @param session What outlives the App: the child server, the Console, the steps of Play online.
    EditorState(ecs::App& app, const std::filesystem::path& layoutFile, std::shared_ptr<EditorSession> session);
    ~EditorState() = default;
    EditorState(const EditorState& other) = delete;
    EditorState& operator=(const EditorState& other) = delete;
    EditorState(EditorState&& other) = delete;
    EditorState& operator=(EditorState&& other) = delete;

    std::shared_ptr<EditorSession> session;    ///< What outlives this App; the members below point into it.
    ecs::App* app;                             ///< The App: its schedules (pause) and its world.
    ecs::World* world;                         ///< What the panels show: the App's world.
    std::unique_ptr<gui::GuiContext> context;  ///< The ImGui context, destroyed with the editor.
    std::shared_ptr<gui::GuiInput> input;      ///< Feeds ImGui; shared with the window's event filter.
    play::PlaySession play;                    ///< Whether the game is being played, and what Stop puts back.
    std::filesystem::path assets;              ///< The game's assets (AssetDirectory); empty: none.

    std::optional<plugins::capsule::CapsuleId> scene;  ///< The scene open; none: no scene is.
    float prefabZoom{2.0F};                     ///< How big the Prefab window shows the capsule: pixels per world unit.
    std::optional<scene::Isolation> prefab;     ///< The capsule shown alone (prefab mode), over the scene; none: not.
    std::optional<std::uint32_t> selectedNode;  ///< The node of the edited capsule the Inspector shows.
    std::vector<plugins::capsule::CapsuleId> unsaved;  ///< The capsules changed since they were saved or opened.
    ecs::Entity selected;                              ///< The entity the Inspector shows while playing.

    AssetAction assetAction{AssetAction::kNone};   ///< What the Assets panel's popup asks for.
    std::string assetTarget;                       ///< The folder or file of assetAction, in the assets.
    bool openAssetPopup{false};                    ///< Whether the popup opens at the end of the panel.
    std::array<char, 256> textField{};             ///< The text the popups and the rename fields edit.
    std::array<char, 64> filter{};                 ///< What the Hierarchy keeps while playing: a component name.
    std::array<char, 64> componentFilter{};        ///< What the Inspector's "Add component" list keeps.
    std::string status;                            ///< The last thing that happened, shown in the menu bar.
    std::string startScene;                        ///< The scene the first frame opens (`editor.scene`); then empty.
    std::string textEditor;                        ///< The command that opens a file in a text editor.
    std::shared_ptr<console::ConsoleLog> console;  ///< The program's output; null if it could not be captured.
    server::ServerSupervisor* supervisor;          ///< The server run as a child process (Server panel); the session's.
    process::ClientLauncher* clients;              ///< The detached clients; the session's.
    server::ServerPanelState& serverPanel;         ///< What the Server panel remembers between frames; the session's.
    std::array<char, 128> consoleFilter{};         ///< What the Console keeps: a part of a line.
    std::size_t consoleSeen{0};                    ///< The lines the Console has shown, to scroll to new ones.
    bool consoleInfo{true};                        ///< Whether the Console shows the other lines.
    bool consoleWarnings{true};                    ///< Whether it shows the warnings.
    bool consoleErrors{true};                      ///< Whether it shows the errors.
    contract::platform::IPlatform* platform{nullptr};    ///< The window of the frame being built: its clipboard.
    std::string clipboard;                               ///< The clipboard's text, kept for ImGui until its next read.
    std::optional<core::asset::AssetId> selectedEffect;  ///< The particle effect the Particles panel edits.
    ecs::Entity preview;                                 ///< The entity playing that effect for a preview; null: none.

    double lastTime{0.0};      ///< The platform's time at the previous frame, in seconds.
    bool buildLayout{true};    ///< Whether the first frame docks the panels as by default, if no layout was kept.
    bool resetLayout{false};   ///< Whether the next frame docks them as by default anyway.
    bool showGame{true};       ///< Whether the Game panel is open.
    bool showHierarchy{true};  ///< Whether the Hierarchy panel is open.
    bool showInspector{true};  ///< Whether the Inspector panel is open.
    bool showAssets{true};     ///< Whether the Assets panel is open.
    bool showStats{true};      ///< Whether the Stats panel is open.
    bool showParticles{true};  ///< Whether the Particles panel is open.
    bool showConsole{true};    ///< Whether the Console panel is open.
    bool showServer{true};     ///< Whether the Server panel is open.
    std::array<float, kFrameHistory> frameTimes{};  ///< The last frames' durations, in milliseconds, as a ring.
    std::size_t frameCursor{0};                     ///< Where the next frame time goes in frameTimes.

    /// @return The capsule the Hierarchy and the Inspector edit: the prefab shown alone, else the scene.
    [[nodiscard]] std::optional<plugins::capsule::CapsuleId> getEdited() const noexcept;

    /// @return Whether the edited capsule changed since it was saved.
    [[nodiscard]] bool isEditedUnsaved() const noexcept;

    /// @brief Notes that the edited capsule changed.
    void markEditedUnsaved();

    /// @brief Notes that the edited capsule was saved.
    void markEditedSaved();
};

}  // namespace rtype::editor
