/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticlePlugin
*/

#pragma once

#include <string_view>

#include "rtype/ecs/app/Plugin.hpp"

namespace rtype::ecs {
class App;
}  // namespace rtype::ecs

namespace rtype::plugins::particle {

/// @brief Particle effects: inserts the ParticleEffectAssets and ParticleSystems resources, registers ParticleEmitter
/// and ParticleOneShot and the particle events, and plays the emitters, on the CPU or the GPU (docs/particles/).
///
/// @details Four systems, all on the client:
/// - PostUpdate "rtype.Particle.Spawn": spawns a one-shot emitter (ParticleOneShot) for each SpawnParticles event; it
///   plays from the next frame, once its commands are applied.
/// - PostUpdate "rtype.Particle.Update", after the TransformPlugin's propagation: starts the emitters with a
///   GlobalTransform whose effect is loaded (restarting the ones whose effect changed), picking the CPU or the GPU
///   (the emitter's override, else the effect's choice; the CPU when the renderer cannot, reported once per effect
///   that asked for the GPU), stops the ones whose ParticleEmitter is gone, destroying their GPU system, counts what
///   each one spawns (computeSpawnCount()) and runs the CPU simulations. It sends ParticlesFinished once a one-shot
///   emitter is done, and despawns it if it has ParticleOneShot.
/// - Render "rtype.Particle.Draw", after the BackendPlugin's camera and the TextPlugin's texts, before its sprites:
///   gives the GPU systems their emission (IRenderer::simulate()), then pushes each emitter whose texture is loaded to
///   the SpriteQueue as a ParticleDraw, sorted with the sprites by ParticleEmitter::layer.
/// The BackendPlugin adds it, after the TextPlugin.
class ParticlePlugin : public ecs::Plugin {
  public:
    static constexpr std::string_view kName = "rtype.Particle";                 ///< The plugin's name.
    static constexpr std::string_view kSpawnSystem = "rtype.Particle.Spawn";    ///< Spawns the one-shot emitters.
    static constexpr std::string_view kUpdateSystem = "rtype.Particle.Update";  ///< Plays the emitters.
    static constexpr std::string_view kDrawSystem = "rtype.Particle.Draw";      ///< Draws the emitters.

    ParticlePlugin() = default;
    ~ParticlePlugin() override = default;
    ParticlePlugin(const ParticlePlugin& other) = default;
    ParticlePlugin& operator=(const ParticlePlugin& other) = default;
    ParticlePlugin(ParticlePlugin&& other) noexcept = default;
    ParticlePlugin& operator=(ParticlePlugin&& other) noexcept = default;

    /// @return "rtype.Particle".
    [[nodiscard]] std::string_view getName() const noexcept override;

    /// @brief Inserts the resources, registers the components and events, and adds the systems.
    void build(ecs::App& app) override;
};

}  // namespace rtype::plugins::particle
