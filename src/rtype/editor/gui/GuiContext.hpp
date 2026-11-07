/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** GuiContext
*/

#pragma once

#include <filesystem>
#include <string>

#include "editor/Export.hpp"

struct ImGuiContext;

namespace rtype::editor::gui {

/// @brief A Dear ImGui context, created for the editor and destroyed with it: the docking and the style are set, and
/// the layout of the panels is kept in a file between two runs.
///
/// @details ImGui keeps its current context in a global, so every use of the context makes it current first
/// (makeCurrent()). The editor has one context per App, and the App is built again on a reload.
class RTYPE_EDITOR_API GuiContext {
  public:
    /// @param layoutFile Where the layout of the panels is read and saved; empty: it is not kept.
    explicit GuiContext(const std::filesystem::path& layoutFile);
    ~GuiContext();
    GuiContext(const GuiContext& other) = delete;
    GuiContext& operator=(const GuiContext& other) = delete;
    GuiContext(GuiContext&& other) = delete;
    GuiContext& operator=(GuiContext&& other) = delete;

    /// @return The context, to hand to code that makes it current itself.
    [[nodiscard]] ImGuiContext* get() const noexcept;

    /// @brief Makes this context the one ImGui calls act on.
    void makeCurrent() const;

  private:
    ImGuiContext* _context;   ///< Owned: created and destroyed through ImGui's C API.
    std::string _layoutFile;  ///< ImGui keeps a pointer to it, so it lives as long as the context.
};

}  // namespace rtype::editor::gui
