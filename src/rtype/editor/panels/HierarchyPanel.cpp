/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** HierarchyPanel
*/

#include <imgui.h>

#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "FieldEditor.hpp"
#include "Panels.hpp"
#include "WorldView.hpp"
#include "core/asset/AssetFiles.hpp"
#include "editor/scene/SceneEdit.hpp"
#include "editor/shell/TextEditor.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "plugins/capsule/format/CapsuleText.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/entity/Name.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
namespace capsule = plugins::capsule;

/// @brief What the tree asked for while it was drawn, applied once it is: changing the capsule while walking it would
/// move its nodes.
struct TreeRequest {
    enum class Kind : std::uint8_t { kNone, kAdd, kInstance, kRemove, kMove };
    Kind kind{Kind::kNone};
    std::uint32_t node{0};    ///< The node acted on: the parent of an added node, the node removed or moved.
    std::uint32_t target{0};  ///< kMove: the new parent.
    std::string path;         ///< kInstance: the capsule file placed.
};

/// @return The text of a payload of the Assets panel.
std::string payloadPath(const ImGuiPayload& payload) {
    return core::asset::normalizePath(std::filesystem::path{
        std::string{static_cast<const char*>(payload.Data), static_cast<std::size_t>(payload.DataSize)}});
}

/// @brief Draws node @p index of @p capsule and its subtree, recording what the user asks for in @p request.
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the scene's tree.
void drawNode(EditorState& state, const capsule::Capsule& capsule, std::uint32_t index, TreeRequest& request) {
    const capsule::CapsuleNode& node = capsule.getNodes()[index];
    const std::vector<std::uint32_t> children = scene::childrenOf(capsule, index);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
                               ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (state.selectedNode == index) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    const std::string name =
        index == 0 ? std::filesystem::path{std::string{capsule.getPath()}}.stem().string() : node.name;
    const std::string label = node.instance ? std::format("{}  [{}]", name, node.instance->path) : name;
    ImGui::PushID(static_cast<int>(index));
    const bool open = ImGui::TreeNodeEx("##node", flags, "%s", label.c_str());
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state.selectedNode = index;
    }
    if (index != 0 && ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload(kNodePayload, &index, sizeof(index));
        ImGui::TextUnformatted(label.c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kNodePayload)) {
            std::uint32_t moved = 0;
            std::memcpy(&moved, payload->Data, sizeof(moved));
            if (!scene::isInSubtree(capsule, index, moved)) {
                request = TreeRequest{.kind = TreeRequest::Kind::kMove, .node = moved, .target = index};
            }
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(kAssetPayload)) {
            const std::string path = payloadPath(*payload);
            if (capsule::format::isCapsuleFile(path) && path != capsule.getPath()) {
                request = TreeRequest{.kind = TreeRequest::Kind::kInstance, .node = index, .path = path};
            }
        }
        ImGui::EndDragDropTarget();
    }
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Add child")) {
            request = TreeRequest{.kind = TreeRequest::Kind::kAdd, .node = index};
        }
        if (ImGui::MenuItem("Delete", nullptr, false, index != 0)) {
            request = TreeRequest{.kind = TreeRequest::Kind::kRemove, .node = index};
        }
        ImGui::EndPopup();
    }
    if (open) {
        for (const std::uint32_t child : children) {
            drawNode(state, capsule, child, request);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

/// @brief Applies what the tree asked for to the scene, and selects what it made.
void apply(EditorState& state, const capsule::Capsule& current, const TreeRequest& request) {
    const ecs::Components& components = state.world->getComponents();
    capsule::Capsule changed{current.getPath()};
    std::uint32_t selected = 0;
    switch (request.kind) {
        case TreeRequest::Kind::kNone:
            return;
        case TreeRequest::Kind::kAdd:
        case TreeRequest::Kind::kInstance: {
            changed = scene::copyCapsule(current, components);
            std::optional<capsule::CapsuleRef> instance;
            std::string name = "Node";
            if (request.kind == TreeRequest::Kind::kInstance) {
                instance = capsule::CapsuleRef{.id = capsule::CapsuleId::fromPath(request.path), .path = request.path};
                name = std::filesystem::path{request.path}.stem().string();
            }
            selected = scene::addChild(changed, request.node, name, std::move(instance));
            break;
        }
        case TreeRequest::Kind::kRemove:
            changed = scene::copyCapsule(current, components, {.removed = request.node});
            break;
        case TreeRequest::Kind::kMove: {
            const std::string target = current.getNodePath(request.target);
            changed = scene::copyCapsule(current, components, {.moved = std::pair{request.node, request.target}});
            const std::uint32_t parent = changed.findNode(target).value_or(0);
            selected = scene::childrenOf(changed, parent).back();
            break;
        }
    }
    scene::commit(*state.world, std::move(changed));
    state.selectedNode = selected;
    state.markEditedUnsaved();
}

/// @brief The tree of the scene being edited.
void drawSceneTree(EditorState& state) {
    const auto* capsules = state.world->getResources().get<capsule::Capsules>();
    const capsule::Capsule* edited = capsules != nullptr ? capsules->find(*state.getEdited()) : nullptr;
    if (edited == nullptr) {
        ImGui::TextUnformatted("The capsule is not loaded.");
        return;
    }
    if (state.prefab) {
        // The way back to the scene, as a breadcrumb: the scene, then the capsule shown alone over it.
        if (ImGui::SmallButton("< Back")) {
            closePrefab(state);
            return;
        }
        ImGui::SameLine();
        const capsule::Capsule* behind = state.scene ? capsules->find(*state.scene) : nullptr;
        ImGui::TextDisabled("%s >", behind != nullptr ? std::string{behind->getPath()}.c_str() : "(no scene)");
        ImGui::SameLine();
    }
    ImGui::TextUnformatted(std::string{edited->getPath()}.c_str());
    if (state.isEditedUnsaved()) {
        ImGui::SameLine();
        ImGui::TextDisabled("(not saved)");
    }
    if (ImGui::SmallButton("Open in text editor")) {
        const std::filesystem::path file = state.assets / std::string{edited->getPath()};
        if (!shell::openInTextEditor(state.textEditor, file)) {
            state.status = std::format("Could not run '{}'", state.textEditor);
        }
    }
    ImGui::Separator();
    TreeRequest request;
    drawNode(state, *edited, 0, request);
    try {
        apply(state, *edited, request);
    } catch (const capsule::exceptions::CapsuleException& error) {
        state.status = error.what();
    }
}

/// @return What the world tree shows for @p entity: its Name, or its index and first components.
std::string labelOf(const ecs::World& world, ecs::Entity entity) {
    if (const auto* name = world.getComponents().get<ecs::Name>(entity); name != nullptr && !name->value.empty()) {
        return std::format("{}  #{}", name->value.view(), entity.getIndex());
    }
    return describeEntity(world, entity);
}

/// @brief Draws @p entity and its children, by their Children.
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the world's hierarchy.
void drawEntityTree(EditorState& state, ecs::Entity entity) {
    const ecs::World& world = *state.world;
    const auto* children = world.getComponents().get<ecs::Children>(entity);
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (children == nullptr || children->entities.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    if (entity == state.selected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    const std::string label = labelOf(world, entity);
    ImGui::PushID(static_cast<int>(entity.getIndex()));
    const bool open = ImGui::TreeNodeEx("##entity", flags, "%s", label.c_str());
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        state.selected = entity;
    }
    if (open) {
        if (children != nullptr) {
            const std::vector<ecs::Entity> below = children->entities;  // drawing may not change it, but stay safe
            for (const ecs::Entity child : below) {
                drawEntityTree(state, child);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

/// @brief The entities of the running world: a tree by their parents, or, with a filter, a flat list.
void drawWorldTree(EditorState& state) {
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputTextWithHint("##filter", "Filter by component", state.filter.data(), state.filter.size());
    ImGui::Separator();
    const std::vector<ecs::Entity> entities = collectEntities(*state.world, state.filter.data());
    if (!ImGui::BeginChild("##entities")) {
        ImGui::EndChild();
        return;
    }
    if (state.filter.front() != '\0') {
        for (const ecs::Entity entity : entities) {
            const std::string label = labelOf(*state.world, entity);
            ImGui::PushID(static_cast<int>(entity.getIndex()));
            if (ImGui::Selectable(label.c_str(), entity == state.selected)) {
                state.selected = entity;
            }
            ImGui::PopID();
        }
    } else {
        for (const ecs::Entity entity : entities) {
            if (state.world->getComponents().get<ecs::Parent>(entity) == nullptr) {
                drawEntityTree(state, entity);
            }
        }
    }
    ImGui::EndChild();
}
}  // namespace

void drawHierarchyPanel(EditorState& state) {
    if (!state.showHierarchy) {
        return;
    }
    if (ImGui::Begin("Hierarchy", &state.showHierarchy)) {
        if (!state.play.isPlaying() && state.getEdited()) {
            drawSceneTree(state);
        } else {
            drawWorldTree(state);
        }
    }
    ImGui::End();
}

}  // namespace rtype::editor::panels
