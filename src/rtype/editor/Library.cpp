/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Library
*/

#include <memory>
#include <optional>
#include <string>

#include "core/config/Settings.hpp"
#include "editor/EditorPlugin.hpp"
#include "rtype/ecs/app/PluginEntry.hpp"

RTYPE_PLUGIN_LIBRARY(registrar) {
    using rtype::editor::EditorPlugin;
    registrar.getPlugins().add(std::string{EditorPlugin::kName}, [](const rtype::core::config::Settings& settings) {
        const rtype::core::config::Settings editor = settings.section("editor");
        editor.checkKeys({"scene", "textEditor"}, "editor");
        return std::make_unique<EditorPlugin>(rtype::editor::EditorOptions{
            .scene = editor.getString("scene", ""),
            .textEditor = editor.getString("textEditor", ""),
        });
    });
}
