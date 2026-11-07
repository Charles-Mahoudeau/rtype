/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** StatsPanel
*/

#include <imgui.h>

#include <format>
#include <string>

#include "Panels.hpp"
#include "rtype/contract/renderer/stats/FrameStats.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
/// @brief Shows a line of text, formatted by std::format.
void line(const std::string& text) { ImGui::TextUnformatted(text.c_str()); }
}  // namespace

void drawStatsPanel(EditorState& state, const contract::renderer::IRenderer& renderer) {
    if (!state.showStats) {
        return;
    }
    if (ImGui::Begin("Stats", &state.showStats)) {
        const float framerate = ImGui::GetIO().Framerate;
        line(std::format("{:.1f} FPS ({:.2f} ms)", framerate, framerate > 0.0F ? 1000.0F / framerate : 0.0F));
        ImGui::PlotLines("##frameTimes", state.frameTimes.data(), static_cast<int>(state.frameTimes.size()),
                         static_cast<int>(state.frameCursor), "Frame time (ms)", 0.0F, 50.0F, ImVec2{0.0F, 64.0F});
        ImGui::Separator();
        const contract::renderer::FrameStats stats = renderer.getFrameStats();
        line(std::format("Sprites and glyphs: {}", stats.sprites));
        line(std::format("Shapes: {}", stats.shapes));
        line(std::format("Overlay items: {}", stats.overlayItems));
        line(std::format("Editor triangles: {}", stats.meshTriangles));
        line(std::format("Textures: {}", stats.textures));
        ImGui::Separator();
        line(std::format("Entities: {}", state.world->getEntityCount()));
        if (const auto size = renderer.getSceneSize()) {
            line(std::format("Game view: {}x{}", size->x, size->y));
        }
    }
    ImGui::End();
}

}  // namespace rtype::editor::panels
