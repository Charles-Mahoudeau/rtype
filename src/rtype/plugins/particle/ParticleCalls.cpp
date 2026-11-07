/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleCalls
*/

#include "plugins/particle/ParticleCalls.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <utility>

#include "core/asset/AssetId.hpp"
#include "plugins/particle/ParticleEffect.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/exceptions/ParticleExceptions.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/contract/renderer/particle/ParticleCurve.hpp"
#include "rtype/contract/renderer/particle/ParticleSystemDesc.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"

namespace rtype::plugins::particle {
namespace {
using contract::renderer::exceptions::ParticleException;

/// @brief One name a script may give for an option, and its value.
template <typename Value>
struct Choice {
    std::string_view name;  ///< As a script writes it.
    Value value;            ///< What it means.
};

/// @return The value of the choice @p name names among @p choices, for the option @p option.
/// @throws ParticleException If none has this name, listing the names it could be.
template <typename Value, std::size_t Count>
[[nodiscard]] Value choose(const std::string_view option, const std::string_view name,
                           const std::array<Choice<Value>, Count>& choices) {
    std::string expected;
    for (const Choice<Value>& choice : choices) {
        if (choice.name == name) {
            return choice.value;
        }
        expected += std::format("{}'{}'", expected.empty() ? "" : ", ", choice.name);
    }
    throw ParticleException{std::format("Unknown {} '{}' (expected {})", option, name, expected)};
}

/// @return Where the particles of @p arguments spawn.
/// @throws ParticleException If its shape is unknown.
[[nodiscard]] contract::renderer::EmitterShape readShape(const BeginEffectArguments& arguments) {
    enum class Kind : std::uint8_t { kPoint, kCircle, kRect };
    static constexpr std::array<Choice<Kind>, 3> kShapes{{{.name = "point", .value = Kind::kPoint},
                                                          {.name = "circle", .value = Kind::kCircle},
                                                          {.name = "rect", .value = Kind::kRect}}};
    switch (choose("shape", arguments.shape.view(), kShapes)) {
        case Kind::kCircle:
            return contract::renderer::CircleShape{.radius = arguments.radius, .edgeOnly = arguments.edgeOnly};
        case Kind::kRect:
            return contract::renderer::RectShape{.size = arguments.rectSize};
        case Kind::kPoint:
            break;
    }
    return contract::renderer::PointShape{};
}

/// @return The effect @p arguments describe, without its bursts and keys.
/// @throws ParticleException If one of its names is unknown.
[[nodiscard]] ParticleEffect readEffect(const BeginEffectArguments& arguments) {
    using contract::renderer::BlendMode;
    using contract::renderer::ParticleAlignment;
    using contract::renderer::ParticleSimulation;
    using contract::renderer::ParticleSpace;
    using contract::renderer::Range;
    static constexpr std::array<Choice<ParticleSimulation>, 3> kSimulations{
        {{.name = "auto", .value = ParticleSimulation::kAuto},
         {.name = "gpu", .value = ParticleSimulation::kGpu},
         {.name = "cpu", .value = ParticleSimulation::kCpu}}};
    static constexpr std::array<Choice<ParticleSpace>, 2> kSpaces{
        {{.name = "world", .value = ParticleSpace::kWorld}, {.name = "local", .value = ParticleSpace::kLocal}}};
    static constexpr std::array<Choice<BlendMode>, 3> kBlends{{{.name = "additive", .value = BlendMode::kAdditive},
                                                               {.name = "alpha", .value = BlendMode::kAlpha},
                                                               {.name = "cutout", .value = BlendMode::kCutout}}};
    static constexpr std::array<Choice<ParticleAlignment>, 2> kAlignments{
        {{.name = "fixed", .value = ParticleAlignment::kFixed},
         {.name = "velocity", .value = ParticleAlignment::kVelocity}}};

    ParticleEffect effect;
    contract::renderer::ParticleSystemDesc& desc = effect.desc;
    desc.simulation = choose("simulation", arguments.simulation.view(), kSimulations);
    desc.capacity = arguments.capacity;
    desc.space = choose("space", arguments.space.view(), kSpaces);
    desc.sortByAge = arguments.sortByAge;
    desc.rate = arguments.rate;
    desc.duration = arguments.duration;
    desc.loop = arguments.loop;
    desc.shape = readShape(arguments);
    desc.lifetime = Range{.min = arguments.lifetimeMin, .max = arguments.lifetimeMax};
    desc.speed = Range{.min = arguments.speedMin, .max = arguments.speedMax};
    desc.angle = Range{.min = arguments.angleMin, .max = arguments.angleMax};
    desc.rotation = Range{.min = arguments.rotationMin, .max = arguments.rotationMax};
    desc.angularVelocity = Range{.min = arguments.angularVelocityMin, .max = arguments.angularVelocityMax};
    desc.size = Range{.min = arguments.sizeMin, .max = arguments.sizeMax};
    desc.color = arguments.color;
    desc.gravity = arguments.gravity;
    desc.drag = arguments.drag;
    effect.texture = arguments.texture;
    effect.source = contract::renderer::math::Rect{arguments.sourcePosition, arguments.sourceSize};
    effect.blend = choose("blend", arguments.blend.view(), kBlends);
    effect.alignment = choose("alignment", arguments.alignment.view(), kAlignments);
    effect.stretch = arguments.stretch;
    effect.flipbook = contract::renderer::ParticleFlipbook{.columns = arguments.flipbookColumns,
                                                           .rows = arguments.flipbookRows,
                                                           .frameCount = arguments.flipbookFrames,
                                                           .cycles = arguments.flipbookCycles};
    effect.material = arguments.material;
    return effect;
}

/// @return The draft named @p name, to fill it.
/// @throws ParticleException If no effect of this name was begun.
[[nodiscard]] ParticleEffect& requireDraft(ParticleEffectDrafts& drafts, const std::string_view name) {
    const auto found = drafts.effects.find(name);
    if (found == drafts.effects.end()) {
        throw ParticleException{
            std::format("No particle effect '{}' was begun (call rtype.particle.beginEffect first)", name)};
    }
    return found->second;
}

void beginEffect(ecs::ResMut<ParticleEffectDrafts> drafts, const BeginEffectArguments& arguments) {
    if (arguments.name.empty()) {
        throw ParticleException{"A particle effect needs a name"};
    }
    if (arguments.flipbookColumns == 0 || arguments.flipbookRows == 0) {
        throw ParticleException{
            std::format("Particle effect '{}': a flipbook has 1 column and 1 row or more", arguments.name.view())};
    }
    drafts->effects.insert_or_assign(std::string{arguments.name.view()}, readEffect(arguments));
}

void addBurst(ecs::ResMut<ParticleEffectDrafts> drafts, const AddBurstArguments& arguments) {
    requireDraft(*drafts, arguments.name.view())
        .desc.bursts.push_back(contract::renderer::ParticleBurst{.time = arguments.time,
                                                                 .count = arguments.count,
                                                                 .cycles = arguments.cycles,
                                                                 .interval = arguments.interval});
}

void addColorKey(ecs::ResMut<ParticleEffectDrafts> drafts, const AddColorKeyArguments& arguments) {
    requireDraft(*drafts, arguments.name.view())
        .desc.colorOverLife.keys.push_back(contract::renderer::GradientKey{.t = arguments.t, .value = arguments.color});
}

void addCurveKey(ecs::ResMut<ParticleEffectDrafts> drafts, const AddCurveKeyArguments& arguments) {
    ParticleEffect& effect = requireDraft(*drafts, arguments.name.view());
    const std::array<Choice<contract::renderer::Curve*>, 2> curves{
        {{.name = "size", .value = &effect.desc.sizeOverLife}, {.name = "speed", .value = &effect.desc.speedOverLife}}};
    choose("curve", arguments.curve.view(), curves)
        ->keys.push_back(contract::renderer::CurveKey{.t = arguments.t, .value = arguments.value});
}

EffectResult addEffect(ecs::ResMut<ParticleEffectDrafts> drafts, ecs::ResMut<ParticleEffectAssets> effects,
                       const AddEffectArguments& arguments) {
    const auto found = drafts->effects.find(arguments.name.view());
    if (found == drafts->effects.end()) {
        throw ParticleException{std::format("No particle effect '{}' was begun (call rtype.particle.beginEffect first)",
                                            arguments.name.view())};
    }
    // Taken out first: an invalid effect is not left behind for the next try.
    auto node = drafts->effects.extract(found);
    try {
        return EffectResult{.effect = effects->add(node.key(), std::move(node.mapped()))};
    } catch (const ParticleException& error) {
        throw ParticleException{std::format("Particle effect '{}': {}", node.key(), error.what())};
    }
}
}  // namespace

void addParticleForms(ecs::PluginRegistry& registry) {
    static constexpr auto kForms = std::to_array<char>({
#include "ParticleForms.luau.h"
    });
    // bin2c ends the bytes with a null terminator, which is not part of the source.
    registry.addScriptForms("addParticleForms", std::string_view{kForms.data(), kForms.size() - 1});
}

void addParticleCalls(ecs::App& app) {
    addParticleForms(app.getPluginRegistry());
    app.addCall(std::string{kBeginEffectCall}, &beginEffect);
    app.addCall(std::string{kAddBurstCall}, &addBurst);
    app.addCall(std::string{kAddColorKeyCall}, &addColorKey);
    app.addCall(std::string{kAddCurveKeyCall}, &addCurveKey);
    app.addCall(std::string{kAddEffectCall}, &addEffect);
}

}  // namespace rtype::plugins::particle
