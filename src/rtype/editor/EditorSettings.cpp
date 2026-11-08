/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorSettings
*/

#include "editor/EditorSettings.hpp"

namespace rtype::editor {
EditorOptions readEditorOptions(const core::config::Settings& settings) {
    const core::config::Settings editor = settings.section("editor");

    editor.checkKeys({"scene", "textEditor"}, "editor");
    return EditorOptions{
        .scene = editor.getString("scene", ""),
        .textEditor = editor.getString("textEditor", ""),
    };
}
}  // namespace rtype::editor
