/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GamePanel
*/

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <optional>

#include "Panels.hpp"
#include "editor/EditorState.hpp"
#include "editor/gui/GameViewport.hpp"
#include "editor/gui/GuiMesh.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"

namespace rtype::editor::panels {

namespace {
/// @return @p length window units as framebuffer pixels, at least 1.
std::uint32_t toPixels(float length, float scale) {
    return static_cast<std::uint32_t>(std::max(1.0F, std::round(length * scale)));
}
}  // namespace

void drawGamePanel(EditorState& state, contract::renderer::IRenderer& renderer) {
    gui::GameViewport viewport;
    if (!state.showGame) {
        renderer.setSceneSize(std::nullopt);
        state.input->setViewport(viewport);
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    const bool visible =
        ImGui::Begin("Game", &state.showGame, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleVar();
    const ImVec2 available = ImGui::GetContentRegionAvail();
    if (visible && available.x >= 1.0F && available.y >= 1.0F) {
        // The scene is drawn as big as the panel, so a pixel of the game is a pixel of the screen.
        const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
        viewport.sceneSize = {toPixels(available.x, scale.x), toPixels(available.y, scale.y)};
        renderer.setSceneSize(viewport.sceneSize);
        const contract::renderer::TextureId scene = renderer.getSceneTexture();
        if (scene != contract::renderer::TextureId{}) {
            ImGui::Image(ImTextureRef{gui::toImGuiTexture(scene)}, available);
            const ImVec2 corner = ImGui::GetItemRectMin();
            viewport.position = {corner.x, corner.y};
            viewport.size = {available.x, available.y};
            viewport.hovered = ImGui::IsItemHovered();
            viewport.focused = ImGui::IsWindowFocused();
        }
    } else {
        renderer.setSceneSize(std::nullopt);
    }
    ImGui::End();
    state.input->setViewport(viewport);
}

}  // namespace rtype::editor::panels
