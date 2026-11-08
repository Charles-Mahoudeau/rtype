/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Theme
*/

#include "Theme.hpp"

#include <imgui.h>

#include <cstdint>
#include <string>

namespace rtype::editor::gui {

namespace {

/// @brief A color from its 0xRRGGBB value, with an optional alpha.
constexpr ImVec4 rgb(std::uint32_t hex, float alpha = 1.0F) {
    return {static_cast<float>((hex >> 16U) & 0xFFU) / 255.0F, static_cast<float>((hex >> 8U) & 0xFFU) / 255.0F,
            static_cast<float>(hex & 0xFFU) / 255.0F, alpha};
}

constexpr ImVec4 withAlpha(ImVec4 color, float alpha) {
    color.w = alpha;
    return color;
}

/// @return @p from moved by @p amount (0 to 1) toward @p to, alpha kept.
constexpr ImVec4 mix(const ImVec4& from, const ImVec4& to, float amount) {
    return {from.x + ((to.x - from.x) * amount), from.y + ((to.y - from.y) * amount),
            from.z + ((to.z - from.z) * amount), from.w};
}

// The Catppuccin Mocha palette: the neutrals, from the deepest layer up, then the colors.
constexpr ImVec4 kCrust = rgb(0x11111B);
constexpr ImVec4 kMantle = rgb(0x181825);
constexpr ImVec4 kBase = rgb(0x1E1E2E);
constexpr ImVec4 kSurface0 = rgb(0x313244);
constexpr ImVec4 kSurface1 = rgb(0x45475A);
constexpr ImVec4 kSurface2 = rgb(0x585B70);
constexpr ImVec4 kOverlay0 = rgb(0x6C7086);
constexpr ImVec4 kOverlay1 = rgb(0x7F849C);
constexpr ImVec4 kSubtext0 = rgb(0xA6ADC8);
constexpr ImVec4 kText = rgb(0xCDD6F4);
constexpr ImVec4 kRosewater = rgb(0xF5E0DC);
constexpr ImVec4 kMauve = rgb(0xCBA6F7);
constexpr ImVec4 kRed = rgb(0xF38BA8);
constexpr ImVec4 kPeach = rgb(0xFAB387);
constexpr ImVec4 kYellow = rgb(0xF9E2AF);
constexpr ImVec4 kGreen = rgb(0xA6E3A1);
constexpr ImVec4 kTeal = rgb(0x94E2D5);
constexpr ImVec4 kBlue = rgb(0x89B4FA);
constexpr ImVec4 kLavender = rgb(0xB4BEFE);

constexpr ImVec4 kAccent = kMauve;
constexpr ImVec4 kClear = rgb(0x000000, 0.0F);

constexpr float kHoverMix = 0.18F;      ///< How much lighter a filled button gets under the pointer.
constexpr float kPressMix = 0.2F;       ///< How much darker it gets pressed.
constexpr float kChipFill = 0.16F;      ///< Alpha of the pill's tint.
constexpr float kChipOutline = 0.5F;    ///< Alpha of the pill's outline.
constexpr float kChipInset = 3.0F;      ///< Space above and below a pill in its row.
constexpr float kChipPadding = 10.0F;   ///< Space left and right of a pill's text.
constexpr float kChipFontScale = 0.5F;  ///< Half: a pill is a capsule, its radius half its height.

ImVec4 colorOf(Tone tone) {
    switch (tone) {
        case Tone::kMuted:
            return kSubtext0;
        case Tone::kAccent:
            return kAccent;
        case Tone::kSuccess:
            return kGreen;
        case Tone::kWarning:
            return kPeach;
        case Tone::kDanger:
            return kRed;
        case Tone::kInfo:
            return kBlue;
        case Tone::kOnline:
            return kTeal;
        case Tone::kText:
        default:
            return kText;
    }
}

void applyColors(ImGuiStyle& style) {
    const auto set = [&style](ImGuiCol index, const ImVec4& color) { style.Colors[index] = color; };

    set(ImGuiCol_Text, kText);
    set(ImGuiCol_TextDisabled, kOverlay0);
    set(ImGuiCol_TextLink, kBlue);
    set(ImGuiCol_TextSelectedBg, withAlpha(kAccent, 0.35F));

    // Depth: the deepest layer is the gaps between panels and the fields (recessed); panels sit on it; bars are
    // in between.
    set(ImGuiCol_WindowBg, kBase);
    set(ImGuiCol_ChildBg, kClear);
    set(ImGuiCol_PopupBg, kMantle);
    set(ImGuiCol_MenuBarBg, kMantle);
    set(ImGuiCol_TitleBg, kMantle);
    set(ImGuiCol_TitleBgActive, kMantle);
    set(ImGuiCol_TitleBgCollapsed, kMantle);
    set(ImGuiCol_DockingEmptyBg, kCrust);
    set(ImGuiCol_Border, kSurface0);
    set(ImGuiCol_BorderShadow, kClear);

    // Inputs are wells, lit by the accent when hovered and focused.
    set(ImGuiCol_FrameBg, kCrust);
    set(ImGuiCol_FrameBgHovered, mix(kCrust, kSurface0, 0.7F));
    set(ImGuiCol_FrameBgActive, kSurface0);
    set(ImGuiCol_CheckMark, kAccent);
    set(ImGuiCol_SliderGrab, kAccent);
    set(ImGuiCol_SliderGrabActive, kLavender);
    set(ImGuiCol_InputTextCursor, kRosewater);

    // Buttons rest on a surface and take the accent when touched.
    set(ImGuiCol_Button, kSurface0);
    set(ImGuiCol_ButtonHovered, mix(kSurface0, kAccent, 0.4F));
    set(ImGuiCol_ButtonActive, mix(kSurface0, kAccent, 0.65F));

    // Selected rows, tree nodes and collapsing headers are tinted with the accent.
    set(ImGuiCol_Header, withAlpha(kAccent, 0.16F));
    set(ImGuiCol_HeaderHovered, withAlpha(kAccent, 0.28F));
    set(ImGuiCol_HeaderActive, withAlpha(kAccent, 0.42F));

    // The splitters are the gaps between panels: dark at rest, the accent when grabbed.
    set(ImGuiCol_Separator, kCrust);
    set(ImGuiCol_SeparatorHovered, kAccent);
    set(ImGuiCol_SeparatorActive, kLavender);
    set(ImGuiCol_ResizeGrip, withAlpha(kSurface2, 0.5F));
    set(ImGuiCol_ResizeGripHovered, withAlpha(kAccent, 0.7F));
    set(ImGuiCol_ResizeGripActive, kAccent);

    // Tabs: the selected one is the panel itself, underlined with the accent.
    set(ImGuiCol_Tab, kMantle);
    set(ImGuiCol_TabHovered, withAlpha(kAccent, 0.35F));
    set(ImGuiCol_TabSelected, kBase);
    set(ImGuiCol_TabSelectedOverline, kAccent);
    set(ImGuiCol_TabDimmed, kMantle);
    set(ImGuiCol_TabDimmedSelected, kBase);
    set(ImGuiCol_TabDimmedSelectedOverline, kOverlay1);
    set(ImGuiCol_DockingPreview, withAlpha(kAccent, 0.45F));

    set(ImGuiCol_ScrollbarBg, kClear);
    set(ImGuiCol_ScrollbarGrab, kSurface1);
    set(ImGuiCol_ScrollbarGrabHovered, kSurface2);
    set(ImGuiCol_ScrollbarGrabActive, kOverlay0);

    set(ImGuiCol_PlotLines, kAccent);
    set(ImGuiCol_PlotLinesHovered, kPeach);
    set(ImGuiCol_PlotHistogram, kTeal);
    set(ImGuiCol_PlotHistogramHovered, kGreen);

    set(ImGuiCol_TableHeaderBg, kMantle);
    set(ImGuiCol_TableBorderStrong, kSurface0);
    set(ImGuiCol_TableBorderLight, withAlpha(kSurface0, 0.6F));
    set(ImGuiCol_TableRowBg, kClear);
    set(ImGuiCol_TableRowBgAlt, withAlpha(kSurface0, 0.22F));
    set(ImGuiCol_TreeLines, kSurface1);

    set(ImGuiCol_DragDropTarget, kYellow);
    set(ImGuiCol_NavCursor, kLavender);
    set(ImGuiCol_NavWindowingHighlight, withAlpha(kText, 0.7F));
    set(ImGuiCol_NavWindowingDimBg, withAlpha(kCrust, 0.6F));
    set(ImGuiCol_ModalWindowDimBg, withAlpha(kCrust, 0.65F));
}

void applyShapes(ImGuiStyle& style) {
    style.FontSizeBase = 15.0F;

    style.WindowPadding = ImVec2{10.0F, 10.0F};
    style.FramePadding = ImVec2{10.0F, 6.0F};
    style.CellPadding = ImVec2{8.0F, 5.0F};
    style.ItemSpacing = ImVec2{8.0F, 7.0F};
    style.ItemInnerSpacing = ImVec2{6.0F, 6.0F};
    style.IndentSpacing = 18.0F;
    style.ScrollbarSize = 12.0F;
    style.GrabMinSize = 12.0F;
    style.SeparatorTextBorderSize = 1.0F;
    style.SeparatorTextPadding = ImVec2{16.0F, 4.0F};
    style.DockingSeparatorSize = 4.0F;

    style.WindowRounding = 8.0F;
    style.ChildRounding = 6.0F;
    style.FrameRounding = 6.0F;
    style.PopupRounding = 8.0F;
    style.TabRounding = 6.0F;
    style.ScrollbarRounding = 12.0F;
    style.GrabRounding = 12.0F;

    style.WindowBorderSize = 1.0F;
    style.ChildBorderSize = 0.0F;
    style.PopupBorderSize = 1.0F;
    style.FrameBorderSize = 0.0F;
    style.TabBorderSize = 0.0F;
    style.TabBarBorderSize = 0.0F;
    style.TabBarOverlineSize = 2.0F;

    style.WindowTitleAlign = ImVec2{0.0F, 0.5F};
    style.WindowMenuButtonPosition = ImGuiDir_None;
}

}  // namespace

void applyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    applyShapes(style);
    applyColors(style);
}

void pushTextTone(Tone tone) { ImGui::PushStyleColor(ImGuiCol_Text, colorOf(tone)); }

void popTextTone() { ImGui::PopStyleColor(); }

void textTone(const std::string& text, Tone tone) {
    pushTextTone(tone);
    ImGui::TextUnformatted(text.c_str());
    popTextTone();
}

bool toneButton(const std::string& label, Tone tone) {
    const ImVec4 color = colorOf(tone);
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, mix(color, kText, kHoverMix));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, mix(color, kCrust, kPressMix));
    ImGui::PushStyleColor(ImGuiCol_Text, kCrust);
    const bool pressed = ImGui::Button(label.c_str());
    ImGui::PopStyleColor(4);
    return pressed;
}

void chip(const std::string& text, Tone tone) {
    const ImVec4 color = colorOf(tone);
    const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    const float rowHeight = ImGui::GetFrameHeight();
    const float height = rowHeight - (2.0F * kChipInset);
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const ImVec2 min{origin.x, origin.y + kChipInset};
    const ImVec2 max{min.x + textSize.x + (2.0F * kChipPadding), min.y + height};
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->AddRectFilled(min, max, ImGui::ColorConvertFloat4ToU32(withAlpha(color, kChipFill)), height * kChipFontScale);
    draw->AddRect(min, max, ImGui::ColorConvertFloat4ToU32(withAlpha(color, kChipOutline)), height * kChipFontScale);
    draw->AddText(ImVec2{min.x + kChipPadding, min.y + ((height - textSize.y) * 0.5F)},
                  ImGui::ColorConvertFloat4ToU32(color), text.c_str());
    ImGui::Dummy(ImVec2{max.x - min.x, rowHeight});
}

}  // namespace rtype::editor::gui
