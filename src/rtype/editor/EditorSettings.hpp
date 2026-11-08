/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorSettings
*/

#pragma once

#include "core/config/Settings.hpp"
#include "editor/EditorState.hpp"
#include "editor/Export.hpp"

namespace rtype::editor {
/// @brief Reads the `editor` section of a project's settings: `scene` and `textEditor`.
/// @param settings The settings `project.luau` gave, the whole of them.
/// @return The options of the editor, the other ones at their defaults.
/// @throws The settings' exception if the section holds another key.
[[nodiscard]] RTYPE_EDITOR_API EditorOptions readEditorOptions(const core::config::Settings& settings);
}  // namespace rtype::editor
