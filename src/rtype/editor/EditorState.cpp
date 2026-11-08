/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorState
*/

#include "EditorState.hpp"

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "editor/EditorSession.hpp"
#include "editor/gui/GuiContext.hpp"
#include "editor/gui/GuiInput.hpp"
#include "plugins/AssetDirectory.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor {

EditorState::EditorState(ecs::App& app, const std::filesystem::path& layoutFile, std::shared_ptr<EditorSession> session)
    : session{std::move(session)},
      app{&app},
      world{&app.getWorld()},
      context{std::make_unique<gui::GuiContext>(layoutFile)},
      input{std::make_shared<gui::GuiInput>(context->get())},
      assets{plugins::assetDirectoryOf(app.getWorld().getResources())},
      console{this->session->console},
      supervisor{this->session->supervisor.get()},
      clients{this->session->clients.get()},
      serverPanel{this->session->serverPanel} {
    if (!assets.empty() && serverPanel.gameDirectory.front() == '\0') {
        const std::string game = assets.parent_path().string();
        const std::size_t length = std::min(game.size(), serverPanel.gameDirectory.size() - 1);
        std::copy_n(game.begin(), length, serverPanel.gameDirectory.begin());
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
