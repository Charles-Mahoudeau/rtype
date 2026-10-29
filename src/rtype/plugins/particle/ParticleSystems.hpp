/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSystems
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <variant>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/particle/CpuSimulation.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

namespace rtype::plugins::particle {

/// @brief The live state of one emitter, kept by ParticleSystems: what changes every frame and gameplay never sees.
struct EmitterState {
    core::asset::AssetId effect;   ///< The effect it was started with: a change restarts it.
    std::uint32_t seed{0};         ///< The seed in use (picked if the component's is 0).
    float time{0.0F};              ///< Seconds since it started, for the bursts, duration and loop.
    float spawnAccumulator{0.0F};  ///< Fraction of a particle carried to the next frame (rate * dt is rarely whole).
    std::uint32_t spawned{0};      ///< Particles emitted so far: the next ParticleEmission::firstParticle.
    bool finished{false};          ///< ParticlesFinished was sent.
    /// The CPU pool, or the GPU system created by the renderer: chosen once, at start, never switched while playing.
    std::variant<CpuSimulation, backend::graphics::ParticleSystemId> simulation;
    /// This frame's emission of a GPU system: computed in PostUpdate, given to IRenderer::simulate() in Render.
    backend::graphics::ParticleEmission emission;
};

/// @brief The live state of every emitter, by entity, as the resource "rtype.ParticleSystems" the ParticlePlugin
/// inserts. Client-side.
///
/// @details A resource rather than a component: the ECS has no removal detection yet, so a system sweeps it every frame
/// and stops the emitters whose entity lost its ParticleEmitter, destroying their GPU system. It only keeps the
/// states: creating and destroying GPU systems is up to the plugin, which has the renderer.
class ParticleSystems {
  public:
    /// @brief The states by emitter entity.
    using Emitters = std::unordered_map<ecs::Entity, EmitterState>;

    ParticleSystems() = default;
    ~ParticleSystems() = default;
    ParticleSystems(const ParticleSystems& other) = delete;
    ParticleSystems& operator=(const ParticleSystems& other) = delete;
    ParticleSystems(ParticleSystems&& other) noexcept = default;
    ParticleSystems& operator=(ParticleSystems&& other) noexcept = default;

    /// @return Every playing emitter, to advance and draw them.
    [[nodiscard]] Emitters& getEmitters() noexcept;
    /// @return Every playing emitter, to advance and draw them.
    [[nodiscard]] const Emitters& getEmitters() const noexcept;

    /// @return The number of playing emitters.
    [[nodiscard]] std::size_t getCount() const noexcept;

    /// @return The state of @p entity's emitter, or nullptr if it is not playing.
    [[nodiscard]] EmitterState* find(ecs::Entity entity);
    /// @return The state of @p entity's emitter, or nullptr if it is not playing.
    [[nodiscard]] const EmitterState* find(ecs::Entity entity) const;

    /// @brief Records that @p entity's emitter started, with @p state. Restarting is stop() then start().
    /// @return The stored state.
    /// @throws std::logic_error If @p entity's emitter is already playing: its GPU system would leak.
    EmitterState& start(ecs::Entity entity, EmitterState&& state);

    /// @brief Forgets @p entity's emitter.
    /// @return Its state, whose GPU system the caller destroys; nothing if it was not playing.
    std::optional<EmitterState> stop(ecs::Entity entity);

    /// @return A new seed for an emitter whose ParticleEmitter::seed is 0: a different one at each call.
    [[nodiscard]] std::uint32_t pickSeed() noexcept;

    /// @brief Records that the fallback of @p effect from the GPU to the CPU was reported, to report it once.
    /// @return Whether it was not reported yet.
    bool markFallbackReported(core::asset::AssetId effect);

  private:
    Emitters _emitters;                                   ///< By emitter entity.
    std::uint32_t _seedCount{0};                          ///< Seeds picked so far.
    std::unordered_set<core::asset::AssetId> _fallbacks;  ///< Effects whose GPU fallback was reported.
};
RTYPE_ECS_RESOURCE(ParticleSystems, "rtype.ParticleSystems");

}  // namespace rtype::plugins::particle
