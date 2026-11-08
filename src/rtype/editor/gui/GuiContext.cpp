/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GuiContext
*/

#include "GuiContext.hpp"

#include <imgui.h>

#include <filesystem>
#include <system_error>

#include "Theme.hpp"

namespace rtype::editor::gui {

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
    applyTheme();
}

GuiContext::~GuiContext() { ImGui::DestroyContext(_context); }

ImGuiContext* GuiContext::get() const noexcept { return _context; }

void GuiContext::makeCurrent() const { ImGui::SetCurrentContext(_context); }

}  // namespace rtype::editor::gui
