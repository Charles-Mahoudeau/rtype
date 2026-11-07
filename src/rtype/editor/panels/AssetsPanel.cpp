/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AssetsPanel
*/

#include <imgui.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include "FieldEditor.hpp"
#include "Panels.hpp"
#include "editor/scene/Isolation.hpp"
#include "editor/scene/SceneEdit.hpp"
#include "editor/shell/TextEditor.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/format/CapsuleText.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
namespace capsule = plugins::capsule;
constexpr const char* kPopup = "Asset##popup";  ///< The popup every action of the panel goes through.

/// @return @p path in the assets, with forward slashes; "" for the assets themselves.
std::string relativeOf(const EditorState& state, const std::filesystem::path& path) {
    const std::string relative = path.lexically_relative(state.assets).generic_string();
    return relative == "." ? std::string{} : relative;
}

/// @return @p folder and @p name joined, in the assets.
std::string join(std::string_view folder, std::string_view name) {
    return folder.empty() ? std::string{name} : std::format("{}/{}", folder, name);
}

/// @brief Asks for the popup of @p action on @p target.
void request(EditorState& state, AssetAction action, std::string target, std::string_view text = {}) {
    state.assetAction = action;
    state.assetTarget = std::move(target);
    state.textField.fill('\0');
    std::memcpy(state.textField.data(), text.data(), std::min(text.size(), state.textField.size() - 1));
    state.openAssetPopup = true;
}

/// @brief Stops editing the capsule @p id, renamed or deleted: its prefab mode ends, or its scene closes.
void forget(EditorState& state, capsule::CapsuleId id) {
    if (state.prefab && state.prefab->capsule == id) {
        closePrefab(state);
    }
    if (state.scene == id) {
        state.scene.reset();
        state.selectedNode.reset();
    }
}

/// @return Whether the capsules know @p path, reading it first if they do not; why not goes to the status line.
bool ensureLoaded(EditorState& state, const std::string& path) {
    const auto* capsules = state.world->getResources().get<capsule::Capsules>();
    if (capsules == nullptr) {
        state.status = "The game has no capsules: list rtype.Capsule in its plugins";
        return false;
    }
    if (capsules->find(capsule::CapsuleId::fromPath(path)) != nullptr) {
        return true;
    }
    if (const std::vector<std::string> errors = scene::readFiles(*state.world, std::span{&path, 1}); !errors.empty()) {
        state.status = errors.front();
        return false;
    }
    return true;
}

/// @brief The menu of a folder: what can be made in it.
void folderMenu(EditorState& state, const std::string& folder) {
    if (ImGui::MenuItem("New scene")) {
        request(state, AssetAction::kNewScene, folder, "level");
    }
    if (ImGui::MenuItem("New actor")) {
        request(state, AssetAction::kNewActor, folder, "actor");
    }
    if (ImGui::MenuItem("New folder")) {
        request(state, AssetAction::kNewFolder, folder, "folder");
    }
}

/// @brief Draws the files and folders of @p folder, folders first, by name.
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the assets' folders.
void drawFolder(EditorState& state, const std::filesystem::path& folder) {
    std::error_code error;
    std::vector<std::filesystem::directory_entry> entries;
    for (const auto& entry : std::filesystem::directory_iterator{folder, error}) {
        if (!entry.path().filename().string().starts_with('.')) {
            entries.push_back(entry);
        }
    }
    std::ranges::sort(entries, [](const auto& left, const auto& right) {
        if (left.is_directory() != right.is_directory()) {
            return left.is_directory();
        }
        return left.path().filename() < right.path().filename();
    });
    for (const auto& entry : entries) {
        const std::string relative = relativeOf(state, entry.path());
        const std::string name = entry.path().filename().string();
        ImGui::PushID(relative.c_str());
        if (entry.is_directory()) {
            const bool open = ImGui::TreeNodeEx("##folder", ImGuiTreeNodeFlags_SpanAvailWidth, "%s/", name.c_str());
            if (ImGui::BeginPopupContextItem()) {
                folderMenu(state, relative);
                ImGui::Separator();
                if (ImGui::MenuItem("Rename")) {
                    request(state, AssetAction::kRename, relative, name);
                }
                if (ImGui::MenuItem("Delete")) {
                    request(state, AssetAction::kDelete, relative);
                }
                ImGui::EndPopup();
            }
            if (open) {
                drawFolder(state, entry.path());
                ImGui::TreePop();
            }
        } else {
            const bool isCapsule = capsule::format::isCapsuleFile(relative);
            const bool isOpen = isCapsule && state.getEdited() == capsule::CapsuleId::fromPath(relative);
            ImGui::TreeNodeEx("##file",
                              ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                  ImGuiTreeNodeFlags_SpanAvailWidth | (isOpen ? ImGuiTreeNodeFlags_Selected : 0),
                              "%s", name.c_str());
            // A scene opens as the level; an actor, or any other capsule, opens alone (prefab mode).
            const bool isScene = std::filesystem::path{relative}.extension() == ".scene";
            if (isCapsule && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (isScene) {
                    openScene(state, relative);
                } else {
                    openPrefab(state, relative);
                }
            }
            if (ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload(kAssetPayload, relative.data(), relative.size());
                ImGui::TextUnformatted(relative.c_str());
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginPopupContextItem()) {
                if (isCapsule && ImGui::MenuItem(isScene ? "Open" : "Open alone")) {
                    if (isScene) {
                        openScene(state, relative);
                    } else {
                        openPrefab(state, relative);
                    }
                }
                if (ImGui::MenuItem("Open in text editor")) {
                    if (!shell::openInTextEditor(state.textEditor, state.assets / relative)) {
                        state.status = std::format("Could not run '{}'", state.textEditor);
                    }
                }
                if (ImGui::MenuItem("Rename")) {
                    request(state, AssetAction::kRename, relative, name);
                }
                if (ImGui::MenuItem("Delete")) {
                    request(state, AssetAction::kDelete, relative);
                }
                ImGui::EndPopup();
            }
        }
        ImGui::PopID();
    }
}

/// @brief Does what the popup asked for, once confirmed.
void runAction(EditorState& state) {
    std::error_code error;
    const std::string text{state.textField.data()};
    const std::filesystem::path target = state.assets / state.assetTarget;
    switch (state.assetAction) {
        case AssetAction::kNewScene:
        case AssetAction::kNewActor: {
            const std::string_view extension = state.assetAction == AssetAction::kNewScene ? ".scene" : ".actor";
            const std::string path =
                join(state.assetTarget, text.ends_with(extension) ? text : text + std::string{extension});
            if (const auto failure = scene::createCapsuleFile(*state.world, state.assets, path)) {
                state.status = *failure;
                return;
            }
            openScene(state, path);
            return;
        }
        case AssetAction::kNewFolder:
            std::filesystem::create_directories(state.assets / join(state.assetTarget, text), error);
            break;
        case AssetAction::kRename: {
            const std::filesystem::path renamed = target.parent_path() / text;
            std::filesystem::rename(target, renamed, error);
            if (!error && capsule::format::isCapsuleFile(renamed)) {
                const std::string path = relativeOf(state, renamed);
                static_cast<void>(scene::readFiles(*state.world, std::span{&path, 1}));
            }
            if (!error) {
                forget(state, capsule::CapsuleId::fromPath(state.assetTarget));  // its capsule keeps the old path
            }
            break;
        }
        case AssetAction::kDelete:
            std::filesystem::remove_all(target, error);
            if (!error) {
                forget(state, capsule::CapsuleId::fromPath(state.assetTarget));
            }
            break;
        case AssetAction::kNone:
            return;
    }
    state.status = error ? std::format("'{}': {}", state.assetTarget, error.message()) : std::string{};
}

/// @brief The popup of the panel's action: a name to type, or a deletion to confirm.
void drawPopup(EditorState& state) {
    if (state.openAssetPopup) {
        ImGui::OpenPopup(kPopup);
        state.openAssetPopup = false;
    }
    if (!ImGui::BeginPopupModal(kPopup, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }
    bool confirmed = false;
    if (state.assetAction == AssetAction::kDelete) {
        ImGui::Text("Delete '%s'? This cannot be undone.", state.assetTarget.c_str());
    } else {
        ImGui::TextUnformatted(state.assetAction == AssetAction::kRename ? "New name" : "Name");
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        confirmed = ImGui::InputText("##name", state.textField.data(), state.textField.size(),
                                     ImGuiInputTextFlags_EnterReturnsTrue);
    }
    confirmed = ImGui::Button("OK") || confirmed;
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        state.assetAction = AssetAction::kNone;
        ImGui::CloseCurrentPopup();
    }
    if (confirmed && (state.assetAction == AssetAction::kDelete || state.textField.front() != '\0')) {
        runAction(state);
        state.assetAction = AssetAction::kNone;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
}  // namespace

void openScene(EditorState& state, const std::string& path) {
    if (state.play.isPlaying()) {
        state.status = "Stop the game before opening a scene";
        return;
    }
    if (!ensureLoaded(state, path)) {
        return;
    }
    closePrefab(state);
    const capsule::CapsuleId id = capsule::CapsuleId::fromPath(path);
    if (const auto error = scene::open(*state.world, id)) {
        state.status = *error;
        return;
    }
    state.scene = id;
    state.selectedNode = 0;
    state.status = std::format("Editing {}", path);
}

void openPrefab(EditorState& state, const std::string& path) {
    if (state.play.isPlaying()) {
        state.status = "Stop the game before opening a capsule";
        return;
    }
    if (!ensureLoaded(state, path)) {
        return;
    }
    closePrefab(state);
    std::string error;
    auto isolation = scene::isolate(*state.world, capsule::CapsuleId::fromPath(path), error);
    if (!isolation) {
        state.status = error;
        return;
    }
    state.prefab = *isolation;
    state.selectedNode = 0;
    state.status = std::format("Editing {} alone", path);
}

void closePrefab(EditorState& state) {
    if (!state.prefab) {
        return;
    }
    scene::endIsolation(*state.world, *state.prefab);
    state.prefab.reset();
    state.selectedNode = state.scene ? std::optional<std::uint32_t>{0} : std::nullopt;
}

void drawAssetsPanel(EditorState& state) {
    if (!state.showAssets) {
        return;
    }
    if (ImGui::Begin("Assets", &state.showAssets)) {
        if (state.assets.empty()) {
            ImGui::TextUnformatted("The game has no assets folder.");
        } else {
            if (ImGui::Button("+")) {
                ImGui::OpenPopup("##newInAssets");
            }
            if (ImGui::BeginPopup("##newInAssets")) {
                folderMenu(state, "");
                ImGui::EndPopup();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("assets/  (double-click a scene to edit it, drag a file onto a node or a field)");
            ImGui::Separator();
            drawFolder(state, state.assets);
        }
        drawPopup(state);
    }
    ImGui::End();
}

}  // namespace rtype::editor::panels
