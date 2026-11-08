/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GuiContext
*/

#include "GuiContext.hpp"

#include <imgui.h>

#include <cstddef>
#include <filesystem>
#include <system_error>

#include "Theme.hpp"

namespace rtype::editor::gui {

namespace {
// JetBrains Mono Nerd Font (SIL OFL 1.1, fonts/OFL.txt): the editor's UI fonts, with the icons of the Nerd Fonts. The
// first one is the text; the second, heavier, is for what is drawn dark on a light fill (see gui::pushEmphasisFont()),
// where thin strokes would look faded. Each array ends with a NUL that bin2c adds, which is not part of the font.
// A plain array: std::to_array of millions of elements would take the compiler far too long.
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
constexpr unsigned char kRegular[] = {
#include "JetBrainsMonoNerdFont-Regular.ttf.h"
};
// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
constexpr unsigned char kBold[] = {
#include "JetBrainsMonoNerdFont-Bold.ttf.h"
};

/// @brief Adds @p data, the bytes of a TTF with a trailing NUL, to the atlas, at the style's base size.
template <std::size_t N>
void addFont(const ImGuiIO& io,
             const unsigned char (&data)[N]) {  // NOLINT(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;  // the bytes live in the executable
    // ImGui reads the bytes and never writes them: its parameter is not const only for the owned-data case.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    io.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(data), static_cast<int>(N - 1), 0.0F, &config);
}

/// @brief Makes the embedded fonts the ones ImGui draws with: the text one first, so it is the default.
void loadFonts(const ImGuiIO& io) {
    addFont(io, kRegular);
    addFont(io, kBold);
}
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
    loadFonts(io);
    applyTheme();
}

GuiContext::~GuiContext() { ImGui::DestroyContext(_context); }

ImGuiContext* GuiContext::get() const noexcept { return _context; }

void GuiContext::makeCurrent() const { ImGui::SetCurrentContext(_context); }

}  // namespace rtype::editor::gui
