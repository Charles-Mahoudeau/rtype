/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorPanel
*/

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <vector>

#include "Panels.hpp"
#include "editor/EditorState.hpp"
#include "editor/gui/GuiMesh.hpp"
#include "editor/gui/Text.hpp"
#include "editor/scene/Isolation.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/view/ScenePreview.hpp"
#include "rtype/contract/renderer/view/View.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace capsule = plugins::capsule;

namespace {
constexpr float kMinZoom = 0.1F;              ///< The farthest the Prefab window shows.
constexpr float kMaxZoom = 16.0F;             ///< The closest.
constexpr float kZoomStep = 1.15F;            ///< How much one notch of the wheel zooms.
constexpr ImVec2 kFirstSize{520.0F, 420.0F};  ///< The window's size the first time it opens.

/// @return @p length window units as pixels, at least 1.
std::uint32_t toPixels(float length, float scale) {
    return static_cast<std::uint32_t>(std::max(1.0F, std::round(length * scale)));
}
}  // namespace

namespace {
/// @brief Draws the tab of the actor @p id; @return whether it asks to close.
/// @param previewed Whether a tab already drew the preview this frame.
bool drawActorTab(EditorState& state, contract::renderer::IRenderer& renderer, capsule::CapsuleId id, bool& previewed) {
    const auto* capsules = state.world->getResources().get<plugins::capsule::Capsules>();
    const auto* shown = capsules != nullptr ? capsules->find(id) : nullptr;
    const std::string path = shown != nullptr ? std::string{shown->getPath()} : std::string{"(gone)"};
    const bool focusing = state.focusActor == id;
    if (focusing) {
        if (state.dockActor) {  // a new tab joins the Game panel's tabs
            const ImGuiWindow* game = ImGui::FindWindowByName("Game");
            if (game != nullptr && game->DockId != 0) {
                ImGui::SetNextWindowDockID(game->DockId, ImGuiCond_Always);
            }
        }
        ImGui::SetNextWindowFocus();
    }
    ImGui::SetNextWindowSize(kFirstSize, ImGuiCond_FirstUseEver);
    bool open = true;
    const std::string title = std::format("Actor: {}###Actor/{}", path, path);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool visible =
        ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    const bool active = state.prefab && state.prefab->capsule == id;
    if (visible && !active && !state.focusActor && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
        openPrefab(state, path);  // the user clicked this tab: it is the actor edited now
    }
    const ImVec2 available = ImGui::GetContentRegionAvail();
    if (visible && state.prefab && state.prefab->capsule == id && available.x >= 1.0F && available.y >= 1.0F) {
        const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
        // The capsule's root in the middle; one world unit is prefabZoom pixels of the window.
        renderer.setScenePreview(contract::renderer::ScenePreview{
            .view = contract::renderer::View{.position = scene::centerOf(*state.world, *state.prefab),
                                             .projection =
                                                 contract::renderer::Orthographic{.zoom = state.prefabZoom * scale.x}},
            .size = {toPixels(available.x, scale.x), toPixels(available.y, scale.y)}});
        previewed = true;
        const contract::renderer::TextureId texture = renderer.getPreviewTexture();
        if (texture != contract::renderer::TextureId{}) {
            ImGui::Image(ImTextureRef{gui::toImGuiTexture(texture)}, available);
            if (ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel != 0.0F) {
                const float factor = std::pow(kZoomStep, ImGui::GetIO().MouseWheel);
                state.prefabZoom = std::clamp(state.prefabZoom * factor, kMinZoom, kMaxZoom);
            }
            ImGui::SetCursorPos(ImVec2{8.0F, 8.0F});
            gui::textDisabled(std::format("zoom x{:.2f} (wheel)", state.prefabZoom));
        }
    }
    ImGui::End();
    return !open;
}
}  // namespace

void drawActorPanel(EditorState& state, contract::renderer::IRenderer& renderer) {
    bool previewed = false;
    const std::vector<capsule::CapsuleId> tabs = state.actors;  // a tab closing changes the list
    std::vector<capsule::CapsuleId> closed;
    for (const capsule::CapsuleId id : tabs) {
        if (drawActorTab(state, renderer, id, previewed)) {
            closed.push_back(id);
        }
    }
    if (!previewed) {
        renderer.setScenePreview(std::nullopt);
    }
    state.focusActor.reset();
    state.dockActor = false;
    for (const capsule::CapsuleId id : closed) {
        closeActor(state, id);  // the tab's cross
    }
}

}  // namespace rtype::editor::panels
