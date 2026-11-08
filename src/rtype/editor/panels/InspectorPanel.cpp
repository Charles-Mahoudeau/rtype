/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** InspectorPanel
*/

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <array>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "FieldEditor.hpp"
#include "Panels.hpp"
#include "WorldView.hpp"
#include "editor/EditorState.hpp"
#include "editor/gui/Text.hpp"
#include "editor/scene/SceneEdit.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/ComponentValues.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
namespace capsule = plugins::capsule;

/// @brief Shows a line of text.
void line(const std::string& text) { ImGui::TextUnformatted(text.c_str()); }

/// @brief Fills the shared text field with @p text, cut to its size.
void fillTextField(EditorState& state, std::string_view text) {
    state.textField.fill('\0');
    std::memcpy(state.textField.data(), text.data(), std::min(text.size(), state.textField.size() - 1));
}

/// @brief The "Add component" button and its list of the components @p has says are missing.
/// @return The component picked, if any.
template <typename Has>
std::optional<ecs::ComponentId> pickComponent(EditorState& state, bool inScene, const Has& has) {
    std::optional<ecs::ComponentId> picked;
    if (ImGui::Button("Add component", ImVec2{-FLT_MIN, 0.0F})) {
        state.componentFilter.fill('\0');
        ImGui::OpenPopup("##addComponent");
    }
    if (!ImGui::BeginPopup("##addComponent")) {
        return picked;
    }
    ImGui::InputTextWithHint("##componentFilter", "Search", state.componentFilter.data(), state.componentFilter.size());
    const std::string wanted{state.componentFilter.data()};
    const ecs::ComponentRegistry& registry = state.world->getComponents().getRegistry();
    for (std::size_t index = 0; index < registry.getCount(); ++index) {
        const auto id = static_cast<ecs::ComponentId>(index);
        const ecs::ComponentInfo* info = registry.findById(id);
        if (info == nullptr || has(id) || isHiddenComponent(info->type.name, inScene) ||
            (!wanted.empty() && !info->type.name.contains(wanted))) {
            continue;
        }
        if (ImGui::Selectable(info->type.name.c_str())) {
            picked = id;
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::EndPopup();
    return picked;
}

/// @brief Shows a component's header, with a button that removes it.
/// @return Whether its fields are shown; @p removed is set if the user removed it.
bool componentHeader(const ecs::ComponentInfo& info, std::size_t index, bool& removed) {
    bool kept = true;
    const std::string title = std::format("{}###component{}", shortName(info.type.name), index);
    const bool open = ImGui::CollapsingHeader(title.c_str(), &kept, ImGuiTreeNodeFlags_DefaultOpen);
    if (ImGui::IsItemHovered()) {
        gui::tooltip(info.type.name);
    }
    removed = !kept;
    return open && kept;
}

/// @brief Shows the name and the side of node @p index of @p edited, which is not the root.
/// @return Whether the user changed them.
bool editNameAndSide(EditorState& state, capsule::Capsule& edited, std::uint32_t index) {
    bool changed = false;
    const capsule::CapsuleNode& node = edited.editNode(index);
    // Filled from the node unless it is being typed in, so the user's typing stays until Enter.
    if (ImGui::GetActiveID() != ImGui::GetID("Name")) {
        fillTextField(state, node.name);
    }
    if (ImGui::InputText("Name", state.textField.data(), state.textField.size(),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (scene::renameNode(edited, index, state.textField.data())) {
            changed = true;
        } else {
            state.status = std::format("'{}' cannot name this node: a sibling has it, or it is empty or has a '/'",
                                       state.textField.data());
        }
    }
    static constexpr std::array<const char*, 3> kSides{"server", "client", "both"};
    static constexpr std::array<ecs::Side, 3> kSideValues{ecs::Side::kServer, ecs::Side::kClient, ecs::Side::kBoth};
    int side = 0;
    for (std::size_t choice = 0; choice < kSideValues.size(); ++choice) {
        if (edited.editNode(index).side == kSideValues.at(choice)) {
            side = static_cast<int>(choice);
        }
    }
    if (ImGui::Combo("Side", &side, kSides.data(), static_cast<int>(kSides.size()))) {
        edited.editNode(index).side = kSideValues.at(static_cast<std::size_t>(side));
        changed = true;
    }
    return changed;
}

/// @brief Shows the components of node @p index of @p edited, changed and removed in place.
/// @return Whether the user changed them.
bool editNodeComponents(const EditorState& state, capsule::Capsule& edited, std::uint32_t index) {
    const ecs::Components& components = state.world->getComponents();
    bool changed = false;
    // The ids first: changing a value while walking the values would move them.
    const capsule::ComponentValues& current = edited.editNode(index).values;
    std::vector<ecs::ComponentId> ids;
    ids.reserve(current.getCount());
    for (std::size_t value = 0; value < current.getCount(); ++value) {
        ids.push_back(current.getId(value));
    }
    for (std::size_t slot = 0; slot < ids.size(); ++slot) {
        const ecs::ComponentId id = ids.at(slot);
        const ecs::ComponentInfo* info = components.getRegistry().findById(id);
        if (info == nullptr || isHiddenComponent(info->type.name, true)) {
            continue;
        }
        bool removed = false;
        capsule::ComponentValues& values = edited.editNode(index).values;
        if (componentHeader(*info, slot, removed)) {
            std::vector<std::byte> copy{values.get(id).begin(), values.get(id).end()};
            ImGui::PushID(static_cast<int>(slot));
            if (editFields(*state.world, info->type, copy)) {
                values.set(components, id, copy);
                changed = true;
            }
            ImGui::PopID();
        }
        if (removed) {
            static_cast<void>(values.remove(id));
            changed = true;
        }
    }
    return changed;
}

/// @brief Shows node @p index of the capsule being edited, @p id: its name, side, instance and components, and applies
/// what the user changes.
void drawNode(EditorState& state, capsule::CapsuleId id, std::uint32_t index) {
    auto capsules = state.world->getResources().getMut<capsule::Capsules>();
    if (!capsules || capsules->find(id) == nullptr) {
        ImGui::TextUnformatted("The capsule is not loaded.");
        return;
    }
    capsule::Capsule& edited = capsules->edit(id);
    if (index >= edited.getNodes().size()) {
        state.selectedNode = 0;
        return;
    }
    const ecs::Components& components = state.world->getComponents();
    line(std::format("Node: {}", index == 0 ? std::string{edited.getPath()} : edited.getNodePath(index)));
    bool changed = index != 0 && editNameAndSide(state, edited, index);
    if (const capsule::CapsuleNode& node = edited.editNode(index); node.instance) {
        line(std::format("Instance of {}", node.instance->path));
    }
    ImGui::Separator();
    changed = editNodeComponents(state, edited, index) || changed;
    ImGui::Separator();
    capsule::ComponentValues& values = edited.editNode(index).values;
    if (const auto picked =
            pickComponent(state, true, [&values](ecs::ComponentId value) { return values.has(value); })) {
        const DefaultValue added{components.getRegistry().findById(*picked)->type};
        values.set(components, *picked, added.getBytes());
        changed = true;
    }
    if (changed) {
        scene::respawn(*state.world, id);
        state.markEditedUnsaved();
    }
}

/// @brief Shows the selected entity of the running world: its components, changed in place, added and removed. What is
/// changed here is lost at Stop.
void drawEntity(EditorState& state) {
    ecs::World& world = *state.world;
    const ecs::Entity entity = state.selected;
    line(std::format("Entity #{} (generation {})", entity.getIndex(), entity.getGeneration()));
    gui::textDisabled("Changes while playing are lost at Stop.");
    ImGui::Separator();
    ecs::Components& components = world.getComponents();
    const std::size_t count = components.getRegistry().getCount();
    std::optional<ecs::ComponentId> removed;
    for (std::size_t index = 0; index < count; ++index) {
        const auto id = static_cast<ecs::ComponentId>(index);
        const ecs::ComponentInfo* info = components.getRegistry().findById(id);
        if (info == nullptr || !components.has(entity, id) || isHiddenComponent(info->type.name, false)) {
            continue;
        }
        bool remove = false;
        if (componentHeader(*info, index, remove)) {
            std::vector<std::byte> copy{components.get(entity, id).begin(), components.get(entity, id).end()};
            ImGui::PushID(static_cast<int>(index));
            if (editFields(world, info->type, copy)) {
                // The copy shares what the value owns, so it is written back whole, through the access that marks it
                // changed.
                const std::span<std::byte> target = components.getMut(entity, id);
                std::memcpy(target.data(), copy.data(), std::min(target.size(), copy.size()));
            }
            ImGui::PopID();
        }
        if (remove) {
            removed = id;
        }
    }
    if (removed) {
        static_cast<void>(components.remove(entity, *removed));
    }
    ImGui::Separator();
    if (const auto picked =
            pickComponent(state, false, [&](ecs::ComponentId id) { return components.has(entity, id); })) {
        const DefaultValue added{components.getRegistry().findById(*picked)->type};
        static_cast<void>(components.add(entity, *picked, added.getBytes()));
    }
}
}  // namespace

void drawInspectorPanel(EditorState& state) {
    if (!state.showInspector) {
        return;
    }
    if (ImGui::Begin("Inspector", &state.showInspector)) {
        const std::optional<plugins::capsule::CapsuleId> edited = state.getEdited();
        if (!state.play.isPlaying() && edited && state.selectedNode) {
            drawNode(state, *edited, *state.selectedNode);
        } else if (!state.selected.isNull() && state.world->isAlive(state.selected)) {
            // Online, the world is the server's copy: a client never writes it. Edit it from the Server panel.
            const bool online = state.session->online.isActive();
            if (online) {
                gui::textDisabled("Online: read-only. Edit the server's world in the Server panel.");
            }
            ImGui::BeginDisabled(online);
            drawEntity(state);
            ImGui::EndDisabled();
        } else {
            ImGui::TextUnformatted(edited ? "Select a node."
                                          : "Open a scene or an actor in the Assets, or select an entity.");
        }
    }
    ImGui::End();
}

}  // namespace rtype::editor::panels
