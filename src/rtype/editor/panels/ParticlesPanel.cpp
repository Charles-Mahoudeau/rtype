/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticlesPanel
*/

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "Panels.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/graphics/VisibleArea.hpp"
#include "plugins/particle/ParticleEffect.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "plugins/particle/ParticleEmitter.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "rtype/ecs/entity/Name.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/type/String.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::editor::panels {

namespace {
namespace particle = plugins::particle;
namespace renderer = contract::renderer;
constexpr float kDragSpeed = 0.05F;  ///< How much a value changes per pixel of a drag.

/// @return The name an effect was added under, or its id.
std::string nameOf(const particle::ParticleEffectAssets& effects, core::asset::AssetId id) {
    const particle::ParticleEffectAssets::Entry* entry = effects.getEntry(id);
    return entry != nullptr ? entry->path : std::format("{:016x}", id.value);
}

/// @brief Edits a range of the description.
bool editRange(const char* label, renderer::Range& range, float lowest = -FLT_MAX) {
    return ImGui::DragFloatRange2(label, &range.min, &range.max, kDragSpeed, lowest, FLT_MAX);
}

/// @brief Edits where the particles spawn.
bool editShape(renderer::EmitterShape& shape) {
    if (auto* circle = std::get_if<renderer::CircleShape>(&shape)) {
        bool changed = ImGui::DragFloat("circle radius", &circle->radius, kDragSpeed, 0.0F, FLT_MAX);
        changed = ImGui::Checkbox("on the edge only", &circle->edgeOnly) || changed;
        return changed;
    }
    if (auto* rect = std::get_if<renderer::RectShape>(&shape)) {
        return ImGui::DragFloat2("rect size", &rect->size.x, kDragSpeed, 0.0F, FLT_MAX);
    }
    ImGui::TextDisabled("shape: a point");
    return false;
}

/// @brief Edits the description the panel shows.
/// @return Whether a value changed.
bool editDesc(renderer::ParticleSystemDesc& desc) {
    bool changed = false;
    ImGui::SeparatorText("Spawning");
    changed = ImGui::DragFloat("rate", &desc.rate, kDragSpeed * 10.0F, 0.0F, FLT_MAX) || changed;
    changed = ImGui::DragFloat("duration", &desc.duration, kDragSpeed, 0.001F, FLT_MAX) || changed;
    changed = ImGui::Checkbox("loop", &desc.loop) || changed;
    for (std::size_t index = 0; index < desc.bursts.size(); ++index) {
        renderer::ParticleBurst& burst = desc.bursts.at(index);
        ImGui::PushID(static_cast<int>(index));
        ImGui::Text("burst %zu", index);
        changed = ImGui::DragFloat("time", &burst.time, kDragSpeed, 0.0F, FLT_MAX) || changed;
        changed = ImGui::DragScalar("count", ImGuiDataType_U32, &burst.count) || changed;
        changed = ImGui::DragScalar("cycles", ImGuiDataType_U32, &burst.cycles) || changed;
        changed = ImGui::DragFloat("interval", &burst.interval, kDragSpeed, 0.0F, FLT_MAX) || changed;
        ImGui::PopID();
    }
    changed = editShape(desc.shape) || changed;
    ImGui::SeparatorText("Each particle");
    changed = editRange("lifetime", desc.lifetime, 0.001F) || changed;
    changed = editRange("speed", desc.speed) || changed;
    changed = editRange("angle", desc.angle) || changed;
    changed = editRange("rotation", desc.rotation) || changed;
    changed = editRange("angularVelocity", desc.angularVelocity) || changed;
    changed = editRange("size", desc.size, 0.0F) || changed;
    changed = ImGui::ColorEdit4("color", &desc.color.x) || changed;
    ImGui::SeparatorText("Forces");
    changed = ImGui::DragFloat2("gravity", &desc.gravity.x, kDragSpeed * 10.0F) || changed;
    changed = ImGui::DragFloat("drag", &desc.drag, kDragSpeed, 0.0F, FLT_MAX) || changed;
    ImGui::TextDisabled("capacity %u; curves, texture and blending are kept as the script made them", desc.capacity);
    return changed;
}

/// @return A Luau range: a number when its ends are equal, `{ min = , max = }` otherwise.
std::string luauRange(const renderer::Range& range) {
    if (range.min == range.max) {
        return std::format("{}", range.min);
    }
    return std::format("{{ min = {}, max = {} }}", range.min, range.max);
}

/// @return The fields of `engine.assets.particle` the panel edits, as the Luau that makes them.
std::string toLuau(const std::string& name, const renderer::ParticleSystemDesc& desc) {
    std::string text = std::format("engine.assets.particle({{\n\tname = \"{}\",\n", name);
    text += std::format("\trate = {},\n\tduration = {},\n\tloop = {},\n", desc.rate, desc.duration, desc.loop);
    if (!desc.bursts.empty()) {
        text += "\tbursts = {";
        for (const renderer::ParticleBurst& burst : desc.bursts) {
            text += std::format(" {{ time = {}, count = {}, cycles = {}, interval = {} }},", burst.time, burst.count,
                                burst.cycles, burst.interval);
        }
        text += " },\n";
    }
    text += std::format("\tlifetime = {},\n\tspeed = {},\n\tangle = {},\n", luauRange(desc.lifetime),
                        luauRange(desc.speed), luauRange(desc.angle));
    text += std::format("\trotation = {},\n\tangularVelocity = {},\n\tsize = {},\n", luauRange(desc.rotation),
                        luauRange(desc.angularVelocity), luauRange(desc.size));
    text += std::format("\tcolor = {{ {}, {}, {}, {} }},\n", desc.color.x, desc.color.y, desc.color.z, desc.color.w);
    text += std::format("\tgravity = {{ x = {}, y = {} }},\n\tdrag = {},\n", desc.gravity.x, desc.gravity.y, desc.drag);
    text += "\t-- and the curves, texture and blending your script gives\n})\n";
    return text;
}

/// @brief Plays the effect @p id at the middle of the view, in place of the previous preview.
void preview(EditorState& state, core::asset::AssetId id) {
    ecs::World& world = *state.world;
    if (!state.preview.isNull() && world.isAlive(state.preview)) {
        static_cast<void>(world.despawn(state.preview));
    }
    glm::vec2 center{0.0F};
    if (const auto* visible = world.getResources().get<plugins::graphics::VisibleArea>();
        visible && !visible->isEmpty()) {
        center = (visible->min + visible->max) * 0.5F;
    }
    state.preview =
        world.spawn(plugins::transform::Transform{.position = center}, particle::ParticleEmitter{.effect = id},
                    ecs::Name{.value = ecs::String{"Particle preview"}});
}
}  // namespace

void drawParticlesPanel(EditorState& state) {
    if (!state.showParticles) {
        return;
    }
    if (!ImGui::Begin("Particles", &state.showParticles)) {
        ImGui::End();
        return;
    }
    auto effects = state.world->getResources().getMut<particle::ParticleEffectAssets>();
    if (!effects) {
        ImGui::TextUnformatted("The game has no particles: list rtype.Particle in its plugins.");
        ImGui::End();
        return;
    }
    std::vector<core::asset::AssetId> ids = effects->getIds();
    std::ranges::sort(ids, {}, [&](core::asset::AssetId id) { return nameOf(*effects, id); });
    if (ImGui::BeginChild("##effects", ImVec2{ImGui::GetContentRegionAvail().x * 0.3F, 0.0F},
                          ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders)) {
        for (const core::asset::AssetId id : ids) {
            const std::string name = nameOf(*effects, id);
            const bool failed = effects->get(id) == nullptr;
            ImGui::BeginDisabled(failed);
            if (ImGui::Selectable(name.c_str(), state.selectedEffect == id)) {
                state.selectedEffect = id;
            }
            ImGui::EndDisabled();
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    if (ImGui::BeginChild("##effect")) {
        const particle::ParticleEffect* current = state.selectedEffect ? effects->get(*state.selectedEffect) : nullptr;
        if (current == nullptr) {
            ImGui::TextUnformatted("Select an effect: a script makes them with engine.assets.particle.");
        } else {
            const core::asset::AssetId id = *state.selectedEffect;
            if (ImGui::Button("Preview")) {
                preview(state, id);
            }
            ImGui::SameLine();
            ImGui::BeginDisabled(state.preview.isNull() || !state.world->isAlive(state.preview));
            if (ImGui::Button("Stop preview")) {
                static_cast<void>(state.world->despawn(state.preview));
                state.preview = {};
            }
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Copy as Luau")) {
                ImGui::SetClipboardText(toLuau(nameOf(*effects, id), current->desc).c_str());
                state.status = "The effect's Luau is on the clipboard";
            }
            ImGui::TextDisabled("Changes last until the game stops: paste the Luau in your script to keep them.");
            particle::ParticleEffect edited = *current;
            if (editDesc(edited.desc)) {
                try {
                    effects->replace(id, std::move(edited));
                } catch (const std::exception& error) {
                    state.status = error.what();
                }
            }
        }
    }
    ImGui::EndChild();
    ImGui::End();
}

}  // namespace rtype::editor::panels
