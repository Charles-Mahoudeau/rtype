/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleEvents
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventTraits.hpp"

namespace rtype::plugins::particle {

/// @brief Plays a one-shot effect at a position, without an entity: the plugin spawns a short-lived emitter, despawned
/// once its particles are gone. For explosions and impacts in one line. Client-side.
struct SpawnParticles {
    core::asset::AssetId effect;     ///< From ParticleEffectAssets::load() or add().
    glm::vec2 position{0.0F, 0.0F};  ///< In world units.
    std::int32_t layer{0};           ///< Draw order among the sprites.
};
RTYPE_ECS_EVENT(SpawnParticles, "rtype.SpawnParticles", effect, position, layer);

/// @brief Sent when a one-shot emitter (loop false) has emitted everything and its last particle died. On the GPU,
/// once its duration plus the longest lifetime has passed, without reading anything back.
struct ParticlesFinished {
    ecs::Entity entity;  ///< The emitter's entity.
};
RTYPE_ECS_EVENT(ParticlesFinished, "rtype.ParticlesFinished", entity);

}  // namespace rtype::plugins::particle
