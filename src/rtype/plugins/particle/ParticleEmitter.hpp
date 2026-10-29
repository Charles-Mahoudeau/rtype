/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleEmitter
*/

#pragma once

#include <cstdint>
#include <type_traits>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"

/// @brief A ParticleSimulation field is reflected as its underlying std::uint8_t.
template <>
struct rtype::ecs::FieldTypeOf<rtype::plugins::backend::graphics::ParticleSimulation>
    : std::integral_constant<rtype::ecs::FieldType, rtype::ecs::FieldType::kU8> {};

namespace rtype::plugins::particle {
/// @brief Plays an effect at its entity's GlobalTransform, as the component "rtype.ParticleEmitter". Client-side.
struct ParticleEmitter {
    core::asset::AssetId effect;     ///< From ParticleEffectAssets::load() or add().
    bool playing{true};              ///< false stops emitting; living particles finish their life.
    std::int32_t layer{0};           ///< Draw order among the sprites.
    float rateScale{1.0F};           ///< Multiplies the effect's rate: more smoke as a ship gets damaged.
    bool overrideSimulation{false};  ///< true: simulation replaces the effect's choice.
    backend::graphics::ParticleSimulation simulation{backend::graphics::ParticleSimulation::kAuto};  ///< If overridden.
    std::uint32_t seed{0};  ///< 0: picked when it starts.
};

RTYPE_ECS_COMPONENT(ParticleEmitter, "rtype.ParticleEmitter", effect, playing, layer, rateScale, overrideSimulation,
                    simulation, seed);

/// @brief Despawns its entity once its ParticleEmitter has finished (ParticlesFinished), as the component
/// "rtype.ParticleOneShot": for fire-and-forget effects, such as the ones SpawnParticles plays. Its effect must not
/// loop, or the entity stays. An entity whose effect cannot be read is despawned too.
struct ParticleOneShot {};
RTYPE_ECS_COMPONENT(ParticleOneShot, "rtype.ParticleOneShot");

}  // namespace rtype::plugins::particle
