/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginsPanel
*/

#include <imgui.h>

#include <format>
#include <string>
#include <vector>

#include "Panels.hpp"
#include "PluginInfo.hpp"
#include "editor/EditorState.hpp"
#include "rtype/ecs/app/App.hpp"

namespace rtype::editor::panels {

namespace {
/// @return The names joined by commas, or "-" if there are none.
std::string joinNames(const std::vector<std::string>& names) {
    std::string joined;
    for (const std::string& name : names) {
        joined += joined.empty() ? name : ", " + name;
    }
    return joined.empty() ? "-" : joined;
}
}  // namespace

void drawPluginsPanel(EditorState& state) {
    if (!state.showPlugins) {
        return;
    }
    if (ImGui::Begin("Plugins", &state.showPlugins)) {
        ImGui::InputTextWithHint("##pluginFilter", "Filter", state.pluginFilter.data(), state.pluginFilter.size());
        const PluginList list = collectPlugins(*state.app, state.pluginFilter.data());
        ImGui::TextUnformatted(std::format("{} plugins loaded", state.app->getPlugins().size()).c_str());
        constexpr ImGuiTableFlags kFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable;
        if (ImGui::BeginTable("##plugins", 4, kFlags)) {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Kind", ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn("Requires");
            ImGui::TableSetupColumn("Required by");
            ImGui::TableHeadersRow();
            for (const PluginRow& row : list.loaded) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(row.name.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(row.isEngine ? "engine" : "game");
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(joinNames(row.requires_).c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(joinNames(row.requiredBy).c_str());
            }
            ImGui::EndTable();
        }
        if (!list.notLoaded.empty() && ImGui::CollapsingHeader("Available, not loaded")) {
            for (const std::string& name : list.notLoaded) {
                ImGui::BulletText("%s", name.c_str());
            }
        }
    }
    ImGui::End();
}

}  // namespace rtype::editor::panels
