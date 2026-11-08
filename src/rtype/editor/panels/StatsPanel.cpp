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
#include "editor/EditorState.hpp"
#include "editor/gui/Theme.hpp"
#include "rtype/contract/renderer/IRenderer.hpp"
#include "rtype/contract/renderer/stats/FrameStats.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
/// @brief Shows a line of text, formatted by std::format.
void line(const std::string& text) { ImGui::TextUnformatted(text.c_str()); }

constexpr float kSmoothFps = 55.0F;    ///< At or above it the frame rate is healthy.
constexpr float kPlayableFps = 30.0F;  ///< Below it the game stutters.

/// @return The tone of a frame rate: green when smooth, orange when it stutters a little, red below.
gui::Tone fpsTone(float framerate) {
    if (framerate >= kSmoothFps) {
        return gui::Tone::kSuccess;
    }
    return framerate >= kPlayableFps ? gui::Tone::kWarning : gui::Tone::kDanger;
}
}  // namespace

void drawStatsPanel(EditorState& state, const contract::renderer::IRenderer& renderer) {
    if (!state.showStats) {
        return;
    }
    if (ImGui::Begin("Stats", &state.showStats)) {
        const float framerate = ImGui::GetIO().Framerate;
        gui::textTone(std::format("{:.1f} FPS ({:.2f} ms)", framerate, framerate > 0.0F ? 1000.0F / framerate : 0.0F),
                      fpsTone(framerate));
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
