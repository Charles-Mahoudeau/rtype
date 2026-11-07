/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConsolePanel
*/

#include <imgui.h>

#include <cfloat>
#include <cstddef>
#include <string>
#include <vector>

#include "Panels.hpp"
#include "editor/console/ConsoleLog.hpp"

namespace rtype::editor::panels {

namespace {
/// @return The colour a line of @p level is written in.
ImVec4 colorOf(console::Level level) {
    switch (level) {
        case console::Level::kError:
            return ImVec4{1.0F, 0.45F, 0.4F, 1.0F};
        case console::Level::kWarning:
            return ImVec4{1.0F, 0.8F, 0.35F, 1.0F};
        default:
            return ImGui::GetStyleColorVec4(ImGuiCol_Text);
    }
}

/// @return Whether a line of @p level is shown.
bool shows(const EditorState& state, console::Level level) {
    switch (level) {
        case console::Level::kError:
            return state.consoleErrors;
        case console::Level::kWarning:
            return state.consoleWarnings;
        default:
            return state.consoleInfo;
    }
}
}  // namespace

void drawConsolePanel(EditorState& state) {
    if (!state.showConsole) {
        return;
    }
    if (!ImGui::Begin("Console", &state.showConsole)) {
        ImGui::End();
        return;
    }
    if (!state.console) {
        ImGui::TextUnformatted("The output could not be captured: read it in the terminal.");
        ImGui::End();
        return;
    }
    if (ImGui::Button("Clear")) {
        state.console->clear();
    }
    ImGui::SameLine();
    ImGui::Checkbox("Info", &state.consoleInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warnings", &state.consoleWarnings);
    ImGui::SameLine();
    ImGui::Checkbox("Errors", &state.consoleErrors);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##consoleFilter", "Filter", state.consoleFilter.data(), state.consoleFilter.size());
    ImGui::Separator();

    const std::string filter{state.consoleFilter.data()};
    std::vector<const console::Line*> shown;
    const std::vector<console::Line> lines = state.console->getLines();
    for (const console::Line& line : lines) {
        if (shows(state, line.level) && (filter.empty() || line.text.contains(filter))) {
            shown.push_back(&line);
        }
    }
    if (ImGui::BeginChild("##consoleLines", ImVec2{0.0F, 0.0F}, ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(shown.size()));
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const console::Line& line = *shown.at(static_cast<std::size_t>(row));
                ImGui::PushID(row);
                ImGui::PushStyleColor(ImGuiCol_Text, colorOf(line.level));
                ImGui::TextUnformatted(line.text.c_str());
                ImGui::PopStyleColor();
                if (ImGui::BeginPopupContextItem("##line")) {
                    if (ImGui::MenuItem("Copy")) {
                        ImGui::SetClipboardText(line.text.c_str());
                    }
                    ImGui::EndPopup();
                }
                ImGui::PopID();
            }
        }
        // New lines keep the view at the bottom, unless the user scrolled up to read.
        const std::size_t added = state.console->getAdded();
        if (added != state.consoleSeen && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - 1.0F) {
            ImGui::SetScrollHereY(1.0F);
        }
        state.consoleSeen = added;
    }
    ImGui::EndChild();
    ImGui::End();
}

}  // namespace rtype::editor::panels
