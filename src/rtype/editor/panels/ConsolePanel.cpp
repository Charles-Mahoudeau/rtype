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
#include "editor/EditorState.hpp"
#include "editor/console/ConsoleLog.hpp"
#include "editor/gui/Theme.hpp"

namespace rtype::editor::panels {

namespace {
/// @return The colour a line of @p level is written in.
gui::Tone toneOf(console::Level level) {
    switch (level) {
        case console::Level::kError:
            return gui::Tone::kDanger;
        case console::Level::kWarning:
            return gui::Tone::kWarning;
        default:
            return gui::Tone::kText;
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

/// @brief Shows @p line, row @p row of the list, in the color of its level, with a menu that copies it.
void drawLine(const console::Line& line, int row) {
    ImGui::PushID(row);
    gui::pushTextTone(toneOf(line.level));
    ImGui::TextUnformatted(line.text.c_str());
    gui::popTextTone();
    if (ImGui::BeginPopupContextItem("##line")) {
        if (ImGui::MenuItem("Copy")) {
            ImGui::SetClipboardText(line.text.c_str());
        }
        ImGui::EndPopup();
    }
    ImGui::PopID();
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
                drawLine(*shown.at(static_cast<std::size_t>(row)), row);
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
