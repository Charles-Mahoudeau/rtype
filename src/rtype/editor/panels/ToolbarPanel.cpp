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
#include "editor/scene/SceneEdit.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "rtype/contract/platform/IPlatform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
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

/// @brief The buttons that play, pause, step and stop the game.
void drawPlayButtons(EditorState& state) {
    ecs::App& app = *state.app;
    if (!state.play.isPlaying()) {
        if (ImGui::Button("Play")) {
            closePrefab(state);  // the game plays its scene, not a capsule shown alone
            state.play.play(app);
            state.selected = {};
            state.status = "Playing";
        }
        return;
    }
    const bool paused = app.getSchedules().isPaused();
    if (ImGui::Button(paused ? "Resume" : "Pause")) {
        state.play.togglePause(app);
    }
    ImGui::BeginDisabled(!paused);
    if (ImGui::Button("Step")) {
        state.play.step(app);
    }
    ImGui::EndDisabled();
    if (ImGui::Button("Stop")) {
        const auto error = state.play.stop(app, state.scene);
        state.selected = {};
        state.status = error ? *error : std::string{"Stopped: back to the scene"};
    }
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
    if (ImGui::BeginMenu("View")) {
        ImGui::MenuItem("Game", nullptr, &state.showGame);
        ImGui::MenuItem("Hierarchy", nullptr, &state.showHierarchy);
        ImGui::MenuItem("Inspector", nullptr, &state.showInspector);
        ImGui::MenuItem("Assets", nullptr, &state.showAssets);
        ImGui::MenuItem("Stats", nullptr, &state.showStats);
        ImGui::MenuItem("Particles", nullptr, &state.showParticles);
        ImGui::MenuItem("Console", nullptr, &state.showConsole);
        ImGui::Separator();
        if (ImGui::MenuItem("Reset layout")) {
            state.resetLayout = true;
            state.showGame = state.showHierarchy = state.showInspector = state.showAssets = state.showStats = true;
            state.showParticles = state.showConsole = true;
        }
        ImGui::EndMenu();
    }
    ImGui::Separator();
    drawPlayButtons(state);
    ImGui::Separator();
    if (state.play.isPlaying()) {
        ImGui::TextUnformatted(state.app->getSchedules().isPaused() ? "Paused" : "Playing");
    } else {
        const char* mode = state.prefab ? "Editing alone" : "Editing";
        ImGui::TextUnformatted(state.isEditedUnsaved() ? std::format("{} (not saved)", mode).c_str() : mode);
    }
    if (!state.status.empty()) {
        ImGui::Separator();
        gui::textDisabled(state.status);
    }
    ImGui::EndMainMenuBar();
}

}  // namespace rtype::editor::panels
