/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorState
*/

#include "EditorState.hpp"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <vector>

#include "editor/console/ConsoleLog.hpp"
#include "editor/console/OutputCapture.hpp"
#include "editor/gui/GuiContext.hpp"
#include "editor/gui/GuiInput.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor {

EditorState::EditorState(ecs::App& app, const std::filesystem::path& layoutFile, bool captureOutput)
    : app{&app},
      world{&app.getWorld()},
      context{std::make_unique<gui::GuiContext>(layoutFile)},
      input{std::make_shared<gui::GuiInput>(context->get())},
      assets{plugins::assetDirectoryOf(app.getWorld().getResources())} {
    if (!captureOutput) {
        return;
    }
    try {
        console = std::make_shared<console::ConsoleLog>();
        capture = std::make_unique<console::OutputCapture>(console);
    } catch (const std::exception& error) {
        console.reset();
        std::cerr << "[editor] warning: the Console shows nothing: " << error.what() << '\n';
    }
}

std::optional<plugins::capsule::CapsuleId> EditorState::getEdited() const noexcept {
    return prefab ? std::optional{prefab->capsule} : scene;
}

bool EditorState::isEditedUnsaved() const noexcept {
    const auto edited = getEdited();
    return edited && std::ranges::find(unsaved, *edited) != unsaved.end();
}

void EditorState::markEditedUnsaved() {
    if (const auto edited = getEdited(); edited && !isEditedUnsaved()) {
        unsaved.push_back(*edited);
    }
}

void EditorState::markEditedSaved() {
    if (const auto edited = getEdited()) {
        std::erase(unsaved, *edited);
    }
}

}  // namespace rtype::editor
