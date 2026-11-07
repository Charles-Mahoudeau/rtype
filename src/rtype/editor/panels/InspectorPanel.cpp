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
#include "editor/scene/SceneEdit.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
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
        ImGui::SetTooltip("%s", info.type.name.c_str());
    }
    removed = !kept;
    return open && kept;
}

/// @brief Shows the node being edited: its name, side, instance and components, and applies what the user changes.
void drawNode(EditorState& state) {
    auto capsules = state.world->getResources().getMut<capsule::Capsules>();
    if (!capsules || capsules->find(*state.getEdited()) == nullptr) {
        ImGui::TextUnformatted("The capsule is not loaded.");
        return;
    }
    capsule::Capsule& edited = capsules->edit(*state.getEdited());
    const std::uint32_t index = *state.selectedNode;
    if (index >= edited.getNodes().size()) {
        state.selectedNode = 0;
        return;
    }
    const ecs::Components& components = state.world->getComponents();
    bool changed = false;
    line(std::format("Node: {}", index == 0 ? std::string{edited.getPath()} : edited.getNodePath(index)));
    capsule::CapsuleNode& node = edited.editNode(index);
    if (index != 0) {
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
            if (node.side == kSideValues.at(choice)) {
                side = static_cast<int>(choice);
            }
        }
        if (ImGui::Combo("Side", &side, kSides.data(), static_cast<int>(kSides.size()))) {
            edited.editNode(index).side = kSideValues.at(static_cast<std::size_t>(side));
            changed = true;
        }
    }
    if (node.instance) {
        line(std::format("Instance of {}", node.instance->path));
    }
    ImGui::Separator();
    // The ids first: changing a value while walking the values would move them.
    std::vector<ecs::ComponentId> ids;
    for (std::size_t value = 0; value < node.values.getCount(); ++value) {
        ids.push_back(node.values.getId(value));
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
    ImGui::Separator();
    capsule::ComponentValues& values = edited.editNode(index).values;
    if (const auto picked = pickComponent(state, true, [&values](ecs::ComponentId id) { return values.has(id); })) {
        const DefaultValue added{components.getRegistry().findById(*picked)->type};
        values.set(components, *picked, added.getBytes());
        changed = true;
    }
    if (changed) {
        scene::respawn(*state.world, *state.getEdited());
        state.markEditedUnsaved();
    }
}

/// @brief Shows the selected entity of the running world: its components, changed in place, added and removed. What is
/// changed here is lost at Stop.
void drawEntity(EditorState& state) {
    ecs::World& world = *state.world;
    const ecs::Entity entity = state.selected;
    line(std::format("Entity #{} (generation {})", entity.getIndex(), entity.getGeneration()));
    ImGui::TextDisabled("Changes while playing are lost at Stop.");
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
                std::span<std::byte> target = components.getMut(entity, id);
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
        if (!state.play.isPlaying() && state.getEdited() && state.selectedNode) {
            drawNode(state);
        } else if (!state.selected.isNull() && state.world->isAlive(state.selected)) {
            drawEntity(state);
        } else {
            ImGui::TextUnformatted(state.getEdited() ? "Select a node."
                                                     : "Open a scene or an actor in the Assets, or select an entity.");
        }
    }
    ImGui::End();
}

}  // namespace rtype::editor::panels
