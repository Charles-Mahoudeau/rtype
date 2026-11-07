/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PrefabPanel
*/

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <string>

#include "Panels.hpp"
#include "editor/gui/GuiMesh.hpp"
#include "editor/scene/Isolation.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/contract/renderer/view/ScenePreview.hpp"
#include "rtype/contract/renderer/view/View.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

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

void drawPrefabPanel(EditorState& state, contract::renderer::IRenderer& renderer) {
    if (!state.prefab) {
        renderer.setScenePreview(std::nullopt);
        return;
    }
    const auto* capsules = state.world->getResources().get<plugins::capsule::Capsules>();
    const auto* shown = capsules != nullptr ? capsules->find(state.prefab->capsule) : nullptr;
    const std::string path = shown != nullptr ? std::string{shown->getPath()} : std::string{"(gone)"};
    // A window of its own, next to the Game panel: it floats the first time, and the user may dock it anywhere.
    ImGui::SetNextWindowSize(kFirstSize, ImGuiCond_FirstUseEver);
    bool open = true;
    const std::string title = std::format("Prefab: {}###Prefab", path);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool visible =
        ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    const ImVec2 available = ImGui::GetContentRegionAvail();
    if (visible && available.x >= 1.0F && available.y >= 1.0F) {
        const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
        // The capsule's root in the middle; one world unit is prefabZoom pixels of the window.
        renderer.setScenePreview(contract::renderer::ScenePreview{
            .view = contract::renderer::View{.position = scene::centerOf(*state.world, *state.prefab),
                                             .projection =
                                                 contract::renderer::Orthographic{.zoom = state.prefabZoom * scale.x}},
            .size = {toPixels(available.x, scale.x), toPixels(available.y, scale.y)}});
        const contract::renderer::TextureId texture = renderer.getPreviewTexture();
        if (texture != contract::renderer::TextureId{}) {
            ImGui::Image(ImTextureRef{gui::toImGuiTexture(texture)}, available);
            if (ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel != 0.0F) {
                const float factor = std::pow(kZoomStep, ImGui::GetIO().MouseWheel);
                state.prefabZoom = std::clamp(state.prefabZoom * factor, kMinZoom, kMaxZoom);
            }
            ImGui::SetCursorPos(ImVec2{8.0F, 8.0F});
            ImGui::TextDisabled("zoom x%.2f (wheel)", static_cast<double>(state.prefabZoom));
        }
    } else {
        renderer.setScenePreview(std::nullopt);
    }
    ImGui::End();
    if (!open) {
        closePrefab(state);  // the window's cross: back to the scene
    }
}

}  // namespace rtype::editor::panels
