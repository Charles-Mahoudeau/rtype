/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConsolePanel
*/

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Panels.hpp"
#include "editor/EditorState.hpp"
#include "editor/console/ConsoleLog.hpp"
#include "editor/gui/Theme.hpp"
#include "editor/console/ScriptPrompt.hpp"

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

/// @brief What the input line's callback reaches.
struct PromptContext {
    EditorState* state;  ///< The editor.
};

/// @brief Replaces the name before the cursor of the input line with @p word.
void replaceWord(ImGuiInputTextCallbackData& data, const std::string& word) {
    const std::string_view before{data.Buf, static_cast<std::size_t>(data.CursorPos)};
    const std::size_t length = console::ScriptPrompt::wordAt(before).size();
    data.DeleteChars(data.CursorPos - static_cast<int>(length), static_cast<int>(length));
    data.InsertChars(data.CursorPos, word.c_str());
}

/// @brief The input line's callback: Tab completes the name before the cursor, Up and Down browse the history.
int onPromptEvent(ImGuiInputTextCallbackData* data) {
    const auto& context = *static_cast<PromptContext*>(data->UserData);
    console::ScriptPrompt& prompt = context.state->session->prompt;
    if (data->EventFlag == ImGuiInputTextFlags_CallbackCompletion) {
        std::size_t hidden = 0;
        const std::string_view before{data->Buf, static_cast<std::size_t>(data->CursorPos)};
        const std::vector<std::string> words = prompt.suggestions(before, hidden);
        const std::string prefix = console::ScriptPrompt::commonPrefix(words);
        // Everything the name could be when some are hidden is longer than the prefix of what is shown: keep it.
        if (!prefix.empty() && hidden == 0 && prefix.size() > console::ScriptPrompt::wordAt(before).size()) {
            replaceWord(*data, prefix);
        } else if (words.size() == 1) {
            replaceWord(*data, words.front());
        }
    } else if (data->EventFlag == ImGuiInputTextFlags_CallbackHistory) {
        const std::string current{data->Buf, static_cast<std::size_t>(data->BufTextLen)};
        const std::optional<std::string> line =
            data->EventKey == ImGuiKey_UpArrow ? prompt.older(current) : prompt.newer();
        if (line) {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, line->c_str());
        }
    }
    return 0;
}

/// @brief Shows the completions of the name being typed, each one a button that puts it in the input line.
void drawSuggestions(EditorState& state) {
    console::ScriptPrompt& prompt = state.session->prompt;
    std::size_t hidden = 0;
    const std::vector<std::string> words =
        state.promptActive ? prompt.suggestions(state.consoleInput.data(), hidden) : std::vector<std::string>{};
    if (words.empty()) {
        return;
    }
    for (const std::string& word : words) {
        if (ImGui::SmallButton(word.c_str())) {
            std::string text{state.consoleInput.data()};
            text.resize(text.size() - console::ScriptPrompt::wordAt(text).size());
            text += word;
            text.resize(std::min(text.size(), state.consoleInput.size() - 1));
            std::ranges::copy(text, state.consoleInput.begin());
            state.consoleInput.at(text.size()) = '\0';
            state.focusPrompt = true;
        }
        ImGui::SameLine();
    }
    if (hidden > 0) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): a constant format.
        ImGui::Text("+%zu", hidden);
    } else {
        ImGui::NewLine();
    }
}

/// @brief The input line: runs what is typed in the game's scripts (see console::ScriptPrompt).
void drawPrompt(EditorState& state) {
    drawSuggestions(state);
    if (std::exchange(state.focusPrompt, false)) {
        ImGui::SetKeyboardFocusHere();
    }
    PromptContext context{.state = &state};
    // NOLINTNEXTLINE(bugprone-signed-bitwise): Dear ImGui flags are ints.
    constexpr ImGuiInputTextFlags kFlags = ImGuiInputTextFlags_EnterReturnsTrue |
                                           ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
    ImGui::SetNextItemWidth(-FLT_MIN);
    const bool entered =
        ImGui::InputTextWithHint("##consoleInput", "Luau: config.player.size.x = 10   (Tab completes)",
                                 state.consoleInput.data(), state.consoleInput.size(), kFlags, onPromptEvent, &context);
    state.promptActive = ImGui::IsItemActive();
    if (entered) {
        state.session->prompt.submit(state.consoleInput.data(), *state.console);
        state.consoleInput.fill('\0');
        ImGui::SetKeyboardFocusHere(-1);  // Enter takes the keyboard from the line: give it back
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
    const bool hasPrompt = state.session->prompt.hasBackend();
    const float footer = hasPrompt ? ImGui::GetFrameHeightWithSpacing() * 2.0F : 0.0F;
    if (ImGui::BeginChild("##consoleLines", ImVec2{0.0F, -footer}, ImGuiChildFlags_None,
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
    if (hasPrompt) {
        drawPrompt(state);
    }
    ImGui::End();
}

}  // namespace rtype::editor::panels
