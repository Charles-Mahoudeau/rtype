/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GuiContext
*/

#include "GuiContext.hpp"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <system_error>

namespace rtype::editor::gui {
namespace {
constexpr float kScaleStep = 0.01F;  ///< A smaller change of scale is not applied: the style is rebuilt for nothing.
constexpr float kMinScale = 0.5F;    ///< Below, the text cannot be read.
constexpr float kMaxScale = 4.0F;    ///< Above, a panel no longer fits a window.
}  // namespace

GuiContext::GuiContext(const std::filesystem::path& layoutFile)
    : _context{ImGui::CreateContext()}, _layoutFile{layoutFile.string()} {
    makeCurrent();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableKeyboard;
    // The renderer draws what the mesh says and lets ImGui make its textures: the font atlas grows by itself.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures;
    io.BackendPlatformName = "rtype-editor";
    io.BackendRendererName = "rtype-editor";
    if (_layoutFile.empty()) {
        io.IniFilename = nullptr;
    } else {
        std::error_code error;
        std::filesystem::create_directories(layoutFile.parent_path(), error);
        io.IniFilename = _layoutFile.c_str();
    }
    ImGui::StyleColorsDark();
}

GuiContext::~GuiContext() { ImGui::DestroyContext(_context); }

ImGuiContext* GuiContext::get() const noexcept { return _context; }

void GuiContext::makeCurrent() const { ImGui::SetCurrentContext(_context); }

void GuiContext::setScale(float scale) {
    if (std::abs(scale - _scale) < kScaleStep) {
        return;
    }
    makeCurrent();
    // From the default style each time: ScaleAllSizes() multiplies what is there, so applying it twice would compound.
    ImGuiStyle style;
    ImGui::StyleColorsDark(&style);
    style.ScaleAllSizes(scale);
    style.FontScaleDpi = scale;
    ImGui::GetStyle() = style;
    _scale = scale;
}

float GuiContext::getScale() const noexcept { return _scale; }

float interfaceScale(float contentScale, float framebufferScale) noexcept {
    const float content = std::isfinite(contentScale) && contentScale > 0.0F ? contentScale : 1.0F;
    const float framebuffer = std::isfinite(framebufferScale) && framebufferScale > 0.0F ? framebufferScale : 1.0F;
    return std::clamp(content / framebuffer, kMinScale, kMaxScale);
}

}  // namespace rtype::editor::gui
