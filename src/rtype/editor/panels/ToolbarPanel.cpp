/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ToolbarPanel
*/

#include <imgui.h>

#include <format>
#include <string>

#include "Panels.hpp"
#include "editor/EditorState.hpp"
#include "editor/gui/Text.hpp"
#include "editor/gui/Theme.hpp"
#include "editor/play/OnlinePlay.hpp"
#include "editor/scene/SceneEdit.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
constexpr float kModeWidth = 110.0F;  ///< Width of the Standalone / Online choice, in pixels.

// Nerd Font icons (Font Awesome, UTF-8 bytes of U+F04B, U+F04C, U+F04D, U+F051), spelled out so the file does not
// depend on the compiler's source charset.
constexpr const char* kPlayIcon = "\xEF\x81\x8B";
constexpr const char* kPauseIcon = "\xEF\x81\x8C";
constexpr const char* kStopIcon = "\xEF\x81\x8D";
constexpr const char* kStepIcon = "\xEF\x81\x91";

/// @return @p label with @p icon in front of it.
std::string withIcon(const char* icon, const std::string& label) { return std::string{icon} + "  " + label; }

/// @brief Writes the scene being edited to its file.
void saveScene(EditorState& state) {
    const auto edited = state.getEdited();
    if (!edited || state.assets.empty()) {
        return;
    }
    if (const auto error = scene::save(*state.world, *edited, state.assets)) {
        state.status = *error;
        return;
    }
    state.markEditedSaved();
    const auto* capsules = state.world->getResources().get<plugins::capsule::Capsules>();
    const auto* saved = capsules != nullptr ? capsules->find(*edited) : nullptr;
    state.status = saved != nullptr ? std::format("Saved {}", saved->getPath()) : std::string{"Saved"};
}

/// @brief The choice between playing standalone (both sides in the editor's world) and online (a client of the server).
void drawModeSelector(const EditorState& state) {
    play::OnlinePlay& online = state.session->online;
    const bool isOnline = online.mode() == play::PlayMode::kOnline;
    ImGui::BeginDisabled(state.play.isPlaying() || online.stage() != play::OnlineStage::kIdle);
    ImGui::SetNextItemWidth(kModeWidth);
    if (ImGui::BeginCombo("##playMode", isOnline ? "Online" : "Standalone")) {
        if (ImGui::Selectable("Standalone", !isOnline)) {
            online.setMode(play::PlayMode::kStandalone);
        }
        if (ImGui::Selectable("Online", isOnline)) {
            online.setMode(play::PlayMode::kOnline);
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        gui::tooltip(isOnline ? "Play starts the server (Server panel) and joins it as a client"
                              : "Play runs both sides of the game in the editor's world");
    }
}

/// @brief Play in online mode: remembers the scene, starts the server if needed, and waits for it.
void playOnline(EditorState& state) {
    const auto* capsules = state.world->getResources().get<plugins::capsule::Capsules>();
    const auto* open = state.scene && capsules != nullptr ? capsules->find(*state.scene) : nullptr;
    state.session->reopenScene = open != nullptr ? std::string{open->getPath()} : std::string{};
    const auto started = state.session->online.begin(*state.supervisor, serverOptionsOf(state));
    state.status = started ? std::string{"Starting the server..."} : started.error();
}

/// @brief The buttons that play, pause, step and stop the game.
void drawPlayButtons(EditorState& state) {
    ecs::App& app = *state.app;
    play::OnlinePlay& online = state.session->online;
    if (online.stage() == play::OnlineStage::kStartingServer) {
        gui::textTone("Starting server...", gui::Tone::kWarning);
        if (gui::toneButton("Cancel", gui::Tone::kDanger)) {
            online.end();
            state.status = "Cancelled";
        }
        return;
    }
    if (!state.play.isPlaying()) {
        if (gui::toneButton(withIcon(kPlayIcon, "Play"), gui::Tone::kSuccess)) {
            closePrefab(state);  // the game plays its scene, not a capsule shown alone
            if (online.mode() == play::PlayMode::kOnline) {
                playOnline(state);
                return;
            }
            state.play.play(app);
            state.selected = {};
            state.status = "Playing";
        }
        return;
    }
    const bool paused = app.getSchedules().isPaused();
    if (gui::toneButton(paused ? withIcon(kPlayIcon, "Resume") : withIcon(kPauseIcon, "Pause"),
                        paused ? gui::Tone::kSuccess : gui::Tone::kWarning)) {
        state.play.togglePause(app);
    }
    ImGui::BeginDisabled(!paused);
    if (gui::toneButton(withIcon(kStepIcon, "Step"), gui::Tone::kInfo)) {
        state.play.step(app);
    }
    ImGui::EndDisabled();
    if (gui::toneButton(withIcon(kStopIcon, "Stop"), gui::Tone::kDanger)) {
        if (online.isActive()) {
            online.end();  // rebuilds the editor standalone; the server keeps running
            state.status = "Stopping: back to the scene";
            return;
        }
        const auto error = state.play.stop(app, state.scene);
        state.selected = {};
        state.status = error ? *error : std::string{"Stopped: back to the scene"};
    }
}
void drawViewMenu(EditorState& state) {
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Game", nullptr, &state.showGame);
        ImGui::MenuItem("Hierarchy", nullptr, &state.showHierarchy);
        ImGui::MenuItem("Inspector", nullptr, &state.showInspector);
        ImGui::MenuItem("Assets", nullptr, &state.showAssets);
        ImGui::MenuItem("Stats", nullptr, &state.showStats);
        ImGui::MenuItem("Particles", nullptr, &state.showParticles);
        ImGui::MenuItem("Console", nullptr, &state.showConsole);
        ImGui::MenuItem("Plugins", nullptr, &state.showPlugins);
        ImGui::MenuItem("Server", nullptr, &state.showServer);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset layout")) {
            state.resetLayout = true;
            state.showGame = state.showHierarchy = state.showInspector = state.showAssets = state.showStats = true;
            state.showParticles = state.showConsole = state.showPlugins = state.showServer = true;
        }
        ImGui::EndMenu();
    }
}

/// @return What the bar says about the run: playing, paused, online, or the scene's edit state.
std::string modeText(const EditorState& state) {
    if (!state.play.isPlaying()) {
        const char* mode = state.prefab ? "Editing alone" : "Editing";
        return state.isEditedUnsaved() ? std::format("{} (not saved)", mode) : std::string{mode};
    }
    const bool paused = state.app->getSchedules().isPaused();
    if (state.session->online.isActive()) {
        return paused ? "Online (paused)" : "Online";
    }
    return paused ? "Paused" : "Playing";
}

/// @return The color of the mode chip: what the editor is doing, at a glance.
gui::Tone modeTone(const EditorState& state) {
    if (!state.play.isPlaying()) {
        return state.isEditedUnsaved() ? gui::Tone::kWarning : gui::Tone::kInfo;
    }
    if (state.app->getSchedules().isPaused()) {
        return gui::Tone::kWarning;
    }
    return state.session->online.isActive() ? gui::Tone::kOnline : gui::Tone::kSuccess;
}

}  // namespace

void drawToolbar(EditorState& state, contract::platform::IPlatform& platform) {
    if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal)) {
        saveScene(state);
    }
    if (!ImGui::BeginMainMenuBar()) {
        return;
    }
    if (ImGui::BeginMenu("File")) {
        if (ImGui::MenuItem("Save", "Ctrl+S", false, state.getEdited().has_value())) {
            saveScene(state);
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit")) {
            platform.close();
        }
        ImGui::EndMenu();
    }
    drawViewMenu(state);
    ImGui::Separator();
    drawModeSelector(state);
    drawPlayButtons(state);
    ImGui::Separator();
    gui::chip(modeText(state), modeTone(state));
    if (const std::string& problem = state.session->online.message(); !problem.empty()) {
        ImGui::Separator();
        gui::textTone(problem, gui::Tone::kDanger);
    }
    if (!state.status.empty()) {
        ImGui::Separator();
        gui::textDisabled(state.status);
    }
    ImGui::EndMainMenuBar();
}

}  // namespace rtype::editor::panels
