/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorSettingsTests
*/

#include <gtest/gtest.h>

#include <string>

#include "core/config/Settings.hpp"
#include "editor/EditorSettings.hpp"
#include "rtype/core/config/exceptions/ConfigExceptions.hpp"

namespace {
using rtype::core::config::Settings;
}  // namespace

TEST(EditorSettings, DefaultsToNoSceneAndTheDefaultTextEditor) {
    const rtype::editor::EditorOptions options = rtype::editor::readEditorOptions(Settings{});

    EXPECT_TRUE(options.scene.empty());
    EXPECT_TRUE(options.textEditor.empty());
}

TEST(EditorSettings, ReadsTheSceneAndTheTextEditor) {
    Settings settings;
    settings.set("editor.scene", std::string{"scenes/level.scene"});
    settings.set("editor.textEditor", std::string{"subl"});

    const rtype::editor::EditorOptions options = rtype::editor::readEditorOptions(settings);

    EXPECT_EQ(options.scene, "scenes/level.scene");
    EXPECT_EQ(options.textEditor, "subl");
}

TEST(EditorSettings, ReadsTheGameWindowTitle) {
    Settings settings;
    settings.set("window.title", std::string{"R-Type Duel"});

    EXPECT_EQ(rtype::editor::readEditorOptions(settings).gameTitle, "R-Type Duel");
    EXPECT_TRUE(rtype::editor::readEditorOptions(Settings{}).gameTitle.empty());
}

TEST(EditorSettings, RefusesAnUnknownKey) {
    Settings settings;
    settings.set("editor.sceen", std::string{"level"});

    EXPECT_ANY_THROW(static_cast<void>(rtype::editor::readEditorOptions(settings)));
}
