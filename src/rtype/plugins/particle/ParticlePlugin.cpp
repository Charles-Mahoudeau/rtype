/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticlePlugin
*/

#include "plugins/particle/ParticlePlugin.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <variant>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"  // IWYU pragma: keep
#include "plugins/backend/graphics/IRenderer.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/backend/graphics/SpriteQueue.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/particle/CpuSimulation.hpp"
#include "plugins/particle/ParticleEffect.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "plugins/particle/ParticleEmitter.hpp"
#include "plugins/particle/ParticleEvents.hpp"
#include "plugins/particle/ParticleSpawn.hpp"
#include "plugins/particle/ParticleSystems.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/Time.hpp"

namespace rtype::plugins::particle {

namespace {
using backend::graphics::ParticleSimulation;
using backend::graphics::ParticleSystemId;

/// @return The renderer, or nullptr before the Startup system created the backend.
[[nodiscard]] backend::graphics::IRenderer* getRenderer(backend::Backend& started) noexcept {
    return started.platform != nullptr ? started.renderer.get() : nullptr;
}

/// @brief Destroys the GPU system of @p state, if it has one.
void release(backend::graphics::IRenderer* renderer, const EmitterState& state) {
    if (const auto* system = std::get_if<ParticleSystemId>(&state.simulation);
        system != nullptr && renderer != nullptr) {
        renderer->destroy(*system);
    }
}

/// @brief Stops @p entity's emitter, destroying its GPU system.
void stopEmitter(ParticleSystems& systems, backend::graphics::IRenderer* renderer, ecs::Entity entity) {
    if (const std::optional<EmitterState> stopped = systems.stop(entity)) {
        release(renderer, *stopped);
    }
}

/// @return Whether @p emitter of @p effect (@p id) runs on the GPU: what it asks for, if @p renderer can.
[[nodiscard]] bool pickGpu(const ParticleEmitter& emitter, const ParticleEffect& effect, core::asset::AssetId id,
                           const backend::graphics::IRenderer* renderer, ParticleSystems& systems) {
    const ParticleSimulation wanted = emitter.overrideSimulation ? emitter.simulation : effect.desc.simulation;
    if (wanted == ParticleSimulation::kCpu) {
        return false;
    }
    const bool canGpu = renderer != nullptr && renderer->supportsGpuParticles();
    if (!canGpu && wanted == ParticleSimulation::kGpu && systems.markFallbackReported(id)) {
        std::cerr << "[ParticlePlugin] The renderer cannot simulate particles: an effect that asks for the GPU runs on "
                     "the CPU\n";
    }
    return canGpu;
}

/// @brief Starts @p entity's emitter of @p effect, on the GPU or the CPU.
EmitterState& startEmitter(ParticleSystems& systems, backend::graphics::IRenderer* renderer, ecs::Entity entity,
                           const ParticleEmitter& emitter, const ParticleEffect& effect) {
    const std::uint32_t seed = emitter.seed != 0 ? emitter.seed : systems.pickSeed();
    if (pickGpu(emitter, effect, emitter.effect, renderer, systems)) {
        try {
            return systems.start(entity, EmitterState{.effect = emitter.effect,
                                                      .seed = seed,
                                                      .simulation = renderer->createParticleSystem(effect.desc)});
        } catch (const std::exception& error) {
            std::cerr << "[ParticlePlugin] " << error.what() << ": the effect runs on the CPU\n";
        }
    }
    return systems.start(
        entity, EmitterState{.effect = emitter.effect, .seed = seed, .simulation = CpuSimulation{effect.desc}});
}

/// @return Whether the emitter of @p state, of @p desc, has emitted everything and its last particle died.
[[nodiscard]] bool isFinished(const backend::graphics::ParticleSystemDesc& desc, const EmitterState& state) {
    if (!isEmissionOver(desc, state.time)) {
        return false;
    }
    if (const auto* simulation = std::get_if<CpuSimulation>(&state.simulation)) {
        return simulation->getCount() == 0;
    }
    return state.time >= desc.duration + desc.lifetime.max;
}

/// @brief PostUpdate, client: spawns a one-shot emitter for each SpawnParticles event.
void spawnParticles(ecs::EventReader<SpawnParticles> requests, ecs::Commands& commands) {
    for (const SpawnParticles& request : requests) {
        static_cast<void>(commands.spawn()
                              .add(transform::Transform{.position = request.position})
                              .add(transform::GlobalTransform{.position = request.position})
                              .add(ParticleEmitter{.effect = request.effect, .layer = request.layer})
                              .add(ParticleOneShot{}));
    }
}

/// @brief What updating an emitter needs, shared by the one-shot emitters and the others.
struct UpdateContext {
    // Not owned: the system's parameters, valid while it runs.
    backend::graphics::IRenderer* renderer;         ///< Null before the backend started.
    const ParticleEffectAssets* effects;            ///< The effects by id.
    ParticleSystems* systems;                       ///< The live states.
    float deltaTime;                                ///< Seconds since the previous frame.
    ecs::EventWriter<ParticlesFinished>* finished;  ///< Where the end of one-shot emitters is told.
    ecs::Commands* commands;                        ///< Despawns the ParticleOneShot entities.
    std::unordered_set<ecs::Entity> playing;        ///< The emitters seen this frame, for the sweep.
};

/// @brief Starts, restarts or advances @p entity's emitter, despawning it once done if it is @p oneShot.
void updateEmitter(UpdateContext& context, ecs::Entity entity, const transform::GlobalTransform& global,
                   const ParticleEmitter& emitter, bool oneShot) {
    const ParticleEffect* effect = context.effects->get(emitter.effect);
    if (effect == nullptr) {
        stopEmitter(*context.systems, context.renderer, entity);
        const ParticleEffectAssets::Entry* entry = context.effects->getEntry(emitter.effect);
        if (oneShot && entry != nullptr && !entry->value) {
            context.commands->despawn(entity);
        }
        return;
    }
    context.playing.insert(entity);
    EmitterState* state = context.systems->find(entity);
    if (state != nullptr && state->effect != emitter.effect) {
        stopEmitter(*context.systems, context.renderer, entity);
        state = nullptr;
    }
    if (state == nullptr) {
        state = &startEmitter(*context.systems, context.renderer, entity, emitter, *effect);
    }

    const std::uint32_t spawnCount =
        emitter.playing ? computeSpawnCount(effect->desc, emitter.rateScale, context.deltaTime, *state) : 0;
    state->emission = backend::graphics::ParticleEmission{.position = global.position,
                                                          .scale = global.scale,
                                                          .rotation = global.rotation,
                                                          .deltaTime = context.deltaTime,
                                                          .seed = state->seed,
                                                          .firstParticle = state->spawned,
                                                          .spawnCount = spawnCount};
    state->spawned += spawnCount;
    if (auto* simulation = std::get_if<CpuSimulation>(&state->simulation)) {
        simulation->step(state->emission);
    }

    if (!state->finished && isFinished(effect->desc, *state)) {
        state->finished = true;
        context.finished->send(ParticlesFinished{.entity = entity});
        if (oneShot) {
            context.commands->despawn(entity);
        }
    }
}

/// @brief PostUpdate, client: starts, stops and advances the emitters, and runs the CPU simulations.
void updateEmitters(
    ecs::NonSendMut<backend::Backend> backend, ecs::Res<ParticleEffectAssets> effects,
    ecs::ResMut<ParticleSystems> systems, ecs::Res<ecs::Time> time,
    ecs::Query<const transform::GlobalTransform, const ParticleEmitter, ecs::Without<ParticleOneShot>> emitters,
    ecs::Query<const transform::GlobalTransform, const ParticleEmitter, ecs::With<ParticleOneShot>> oneShots,
    ecs::EventWriter<ParticlesFinished> finished, ecs::Commands& commands) {
    UpdateContext context{.renderer = getRenderer(*backend),
                          .effects = &*effects,
                          .systems = &*systems,
                          .deltaTime = time->delta,
                          .finished = &finished,
                          .commands = &commands,
                          .playing = {}};
    for (auto [entity, global, emitter] : emitters) {
        updateEmitter(context, entity, global, emitter, false);
    }
    for (auto [entity, global, emitter] : oneShots) {
        updateEmitter(context, entity, global, emitter, true);
    }

    std::vector<ecs::Entity> gone;
    for (const auto& [entity, state] : systems->getEmitters()) {
        if (!context.playing.contains(entity)) {
            gone.push_back(entity);
        }
    }
    for (const ecs::Entity entity : gone) {
        stopEmitter(*systems, context.renderer, entity);
    }
}

/// @brief Render, client: simulates the GPU systems and pushes every emitter to the SpriteQueue.
void drawEmitters(ecs::NonSendMut<backend::Backend> backend, ecs::Res<ParticleEffectAssets> effects,
                  ecs::Res<backend::TextureAssets> textures, ecs::ResMut<ParticleSystems> systems,
                  ecs::ResMut<backend::graphics::SpriteQueue> queue, ecs::Query<const ParticleEmitter> emitters) {
    backend::graphics::IRenderer* renderer = getRenderer(*backend);
    const bool framed = renderer != nullptr && backend->platform->getFramebufferSize().x != 0 &&
                        backend->platform->getFramebufferSize().y != 0;
    for (auto [entity, emitter] : emitters) {
        EmitterState* state = systems->find(entity);
        const ParticleEffect* effect = effects->get(emitter.effect);
        if (state == nullptr || effect == nullptr || !framed) {
            continue;
        }
        backend::graphics::ParticleDraw draw;
        if (const auto* system = std::get_if<ParticleSystemId>(&state->simulation)) {
            renderer->simulate(*system, state->emission);
            draw.particles = *system;
        } else {
            const auto& simulation = std::get<CpuSimulation>(state->simulation);
            if (simulation.getCount() == 0) {
                continue;
            }
            draw.particles = simulation.getInstances();
        }
        const std::optional<backend::graphics::TextureId> texture = textures->getTexture(effect->texture);
        if (!texture) {
            continue;
        }
        draw.texture = *texture;
        draw.source = effect->source;
        draw.blend = effect->blend;
        draw.alignment = effect->alignment;
        queue->push(emitter.layer, draw);
    }
}
}  // namespace

std::string_view ParticlePlugin::getName() const noexcept { return kName; }

void ParticlePlugin::build(ecs::App& app) {
    app.addPlugin(transform::TransformPlugin{});
    ecs::Resources& resources = app.getWorld().getResources();
    if (!resources.has<ParticleEffectAssets>()) {
        resources.insert(ParticleEffectAssets{});
    }
    if (!resources.has<ParticleSystems>()) {
        resources.insert(ParticleSystems{});
    }
    if (!resources.has<backend::TextureAssets>()) {
        resources.insert(backend::TextureAssets{});
    }
    if (!resources.has<backend::graphics::SpriteQueue>()) {
        resources.insert(backend::graphics::SpriteQueue{});
    }
    static_cast<void>(app.getWorld().getComponents().registerType<ParticleEmitter>());
    static_cast<void>(app.getWorld().getComponents().registerType<ParticleOneShot>());
    static_cast<void>(app.getWorld().getEvents().registerType<SpawnParticles>());
    static_cast<void>(app.getWorld().getEvents().registerType<ParticlesFinished>());

    app.addSystem(ecs::Phase::kPostUpdate, std::string{kSpawnSystem}, &spawnParticles).setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kPostUpdate, std::string{kUpdateSystem}, &updateEmitters)
        .after(std::string{transform::TransformPlugin::kPropagateSystem})
        .after(std::string{kSpawnSystem})
        .setSide(ecs::Side::kClient);
    app.addSystem(ecs::Phase::kRender, std::string{kDrawSystem}, &drawEmitters)
        .after(std::string{backend::BackendPlugin::kCameraSystem})
        .after(std::string{text::TextPlugin::kDrawSystem})
        .before(std::string{backend::BackendPlugin::kSpritesSystem})
        .setSide(ecs::Side::kClient);
}

}  // namespace rtype::plugins::particle
