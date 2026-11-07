/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleCalls
*/

#pragma once

#include <cstdint>
#include <functional>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <map>
#include <numbers>
#include <string>
#include <string_view>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "plugins/particle/ParticleEffect.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/app/PluginRegistry.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::particle {

inline constexpr std::string_view kBeginEffectCall = "rtype.particle.beginEffect";  ///< Starts a ParticleEffectDraft.
inline constexpr std::string_view kAddBurstCall = "rtype.particle.addBurst";        ///< Adds a burst to a draft.
inline constexpr std::string_view kAddColorKeyCall = "rtype.particle.addColorKey";  ///< Adds to its colorOverLife.
inline constexpr std::string_view kAddCurveKeyCall = "rtype.particle.addCurveKey";  ///< Adds to its size or speed.
inline constexpr std::string_view kAddEffectCall = "rtype.particle.addEffect";      ///< ParticleEffectAssets::add().

/// @brief The effects a script is describing, by name: `rtype.particle.beginEffect` starts one, the `add*Key` and
/// `addBurst` calls fill it, and `rtype.particle.addEffect` checks it and moves it to the ParticleEffectAssets. An
/// effect is built whole before it is stored, so a mistake in its description never leaves half of it playing.
/// The ECS resource "rtype.ParticleEffectDrafts", inserted by the ParticlePlugin.
struct ParticleEffectDrafts {
    std::map<std::string, ParticleEffect, std::less<>> effects;  ///< Begun and not added yet.

    ParticleEffectDrafts() = default;
    ~ParticleEffectDrafts() = default;
    ParticleEffectDrafts(const ParticleEffectDrafts& other) = delete;
    ParticleEffectDrafts& operator=(const ParticleEffectDrafts& other) = delete;
    // noexcept, as a resource must be: MSVC's std::map does not promise it on its own.
    ParticleEffectDrafts(ParticleEffectDrafts&& other) noexcept = default;
    ParticleEffectDrafts& operator=(ParticleEffectDrafts&& other) noexcept = default;
};
RTYPE_ECS_RESOURCE(ParticleEffectDrafts, "rtype.ParticleEffectDrafts");

/// @brief The arguments of `rtype.particle.beginEffect`: everything of an effect but its lists, which the calls below
/// add. Each default is the one of ParticleEffect. Beginning a name again starts it over.
struct BeginEffectArguments {
    ecs::String name;                ///< What the effect is known by: its id is AssetId::fromPath(name).
    ecs::String simulation{"auto"};  ///< "auto", "gpu" or "cpu".
    std::uint32_t capacity{1024};    ///< Most particles alive at once.
    ecs::String space{"world"};      ///< "world": left behind; "local": follow the emitter.
    bool sortByAge{false};           ///< Draws the oldest first; always on with the "alpha" blend.
    float rate{0.0F};                ///< Particles per second while playing.
    float duration{1.0F};            ///< Seconds of one cycle.
    bool loop{true};                 ///< true: a new cycle starts after duration.
    ecs::String shape{"point"};      ///< Where particles spawn: "point", "circle" or "rect".
    float radius{0.0F};              ///< "circle": its radius, in world units.
    bool edgeOnly{false};            ///< "circle": on the circle only, as a ring.
    glm::vec2 rectSize{0.0F, 0.0F};  ///< "rect": its width and height, in world units.
    float lifetimeMin{1.0F};         ///< Seconds; above 0.
    float lifetimeMax{1.0F};         ///< Seconds; lifetimeMin or more.
    float speedMin{0.0F};            ///< World units per second, along the angle.
    float speedMax{0.0F};            ///< speedMin or more.
    float angleMin{0.0F};            ///< Direction, in radians.
    float angleMax{2.0F * std::numbers::pi_v<float>};  ///< Every way by default.
    float rotationMin{0.0F};                           ///< Of the quad, in radians, clockwise on screen.
    float rotationMax{0.0F};                           ///< rotationMin or more.
    float angularVelocityMin{0.0F};                    ///< Of the quad, in radians per second.
    float angularVelocityMax{0.0F};                    ///< angularVelocityMin or more.
    float sizeMin{1.0F};                               ///< Width and height of the quad, in world units.
    float sizeMax{1.0F};                               ///< sizeMin or more.
    glm::vec4 color{1.0F, 1.0F, 1.0F, 1.0F};           ///< Multiplied by colorOverLife.
    glm::vec2 gravity{0.0F, 0.0F};                     ///< World units per second squared; +Y is down.
    float drag{0.0F};                                  ///< Slows particles: velocity times exp(-drag * seconds).
    core::asset::AssetId texture;                      ///< From `engine.assets.texture`.
    glm::vec2 sourcePosition{0.0F, 0.0F};              ///< Top left of the part of the texture shown, in pixels.
    glm::vec2 sourceSize{0.0F, 0.0F};                  ///< Its size; 0: the whole texture.
    ecs::String blend{"additive"};                     ///< "additive", "alpha" or "cutout".
    ecs::String alignment{"fixed"};                    ///< "fixed", or "velocity": turned along the velocity.
    float stretch{0.0F};                               ///< "velocity": the quad grows by speed * stretch.
    std::uint32_t flipbookColumns{1};                  ///< Frames per row of the source.
    std::uint32_t flipbookRows{1};                     ///< Rows of frames.
    std::uint32_t flipbookFrames{0};                   ///< Frames played; 0: every cell.
    float flipbookCycles{1.0F};                        ///< Times the frames play over a life.
    core::asset::AssetId material;                     ///< From `engine.assets.material`; the default one if not given.
};
RTYPE_ECS_CALL_DATA(BeginEffectArguments, "rtype.particle.BeginEffectArguments", name, simulation, capacity, space,
                    sortByAge, rate, duration, loop, shape, radius, edgeOnly, rectSize, lifetimeMin, lifetimeMax,
                    speedMin, speedMax, angleMin, angleMax, rotationMin, rotationMax, angularVelocityMin,
                    angularVelocityMax, sizeMin, sizeMax, color, gravity, drag, texture, sourcePosition, sourceSize,
                    blend, alignment, stretch, flipbookColumns, flipbookRows, flipbookFrames, flipbookCycles, material);

/// @brief The arguments of `rtype.particle.addBurst`: particles emitted at once, in each cycle.
struct AddBurstArguments {
    ecs::String name;         ///< The effect begun with `rtype.particle.beginEffect`.
    float time{0.0F};         ///< Seconds after the start of the cycle.
    std::uint32_t count{0};   ///< Particles emitted each time.
    std::uint32_t cycles{1};  ///< How many times it happens.
    float interval{0.0F};     ///< Seconds between two times.
};
RTYPE_ECS_CALL_DATA(AddBurstArguments, "rtype.particle.AddBurstArguments", name, time, count, cycles, interval);

/// @brief The arguments of `rtype.particle.addColorKey`: a key of the effect's colorOverLife, after the others.
struct AddColorKeyArguments {
    ecs::String name;                         ///< The effect begun with `rtype.particle.beginEffect`.
    float t{0.0F};                            ///< 0 at birth, 1 at death.
    glm::vec4 color{1.0F, 1.0F, 1.0F, 1.0F};  ///< The colour at t, alpha included.
};
RTYPE_ECS_CALL_DATA(AddColorKeyArguments, "rtype.particle.AddColorKeyArguments", name, t, color);

/// @brief The arguments of `rtype.particle.addCurveKey`: a key of the effect's sizeOverLife or speedOverLife.
struct AddCurveKeyArguments {
    ecs::String name;           ///< The effect begun with `rtype.particle.beginEffect`.
    ecs::String curve{"size"};  ///< "size" or "speed".
    float t{0.0F};              ///< 0 at birth, 1 at death.
    float value{1.0F};          ///< The multiplier at t.
};
RTYPE_ECS_CALL_DATA(AddCurveKeyArguments, "rtype.particle.AddCurveKeyArguments", name, curve, t, value);

/// @brief The arguments of `rtype.particle.addEffect`.
struct AddEffectArguments {
    ecs::String name;  ///< The effect begun with `rtype.particle.beginEffect`.
};
RTYPE_ECS_CALL_DATA(AddEffectArguments, "rtype.particle.AddEffectArguments", name);

/// @brief The result of `rtype.particle.addEffect`.
struct EffectResult {
    core::asset::AssetId effect;  ///< For a ParticleEmitter or a SpawnParticles.
};
RTYPE_ECS_CALL_DATA(EffectResult, "rtype.particle.EffectResult", effect);

/// @brief Registers the particle engine calls on @p app, for scripts: `rtype.particle.beginEffect`, `addBurst`,
/// `addColorKey`, `addCurveKey` and `addEffect`. They throw a contract::renderer::exceptions::ParticleException
/// naming what is wrong (an unknown name or draft, an invalid effect). Called by the ParticlePlugin.
void addParticleCalls(ecs::App& app);

/// @brief Registers the setup forms of the plugin (`engine.assets.particle`) on @p registry: its library does it on
/// every side, so a server gets the ids of what it does not build, and so does building the plugin.
void addParticleForms(ecs::PluginRegistry& registry);

}  // namespace rtype::plugins::particle
