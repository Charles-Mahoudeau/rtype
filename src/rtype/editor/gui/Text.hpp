/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Text
*/

#pragma once

#include <string>

#include "editor/Export.hpp"

namespace rtype::editor::gui {

/// @brief Shows @p text greyed out, like ImGui::TextDisabled() but without a format string (the C-style variadic
/// functions of Dear ImGui are not used: a format and its arguments are not checked against each other).
RTYPE_EDITOR_API void textDisabled(const std::string& text);

/// @brief Shows @p text in a tooltip, like ImGui::SetTooltip() but without a format string.
RTYPE_EDITOR_API void tooltip(const std::string& text);

}  // namespace rtype::editor::gui
