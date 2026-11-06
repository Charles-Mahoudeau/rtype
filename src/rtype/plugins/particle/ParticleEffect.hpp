/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleEffect
*/

#pragma once

#include "core/asset/AssetId.hpp"
#include "rtype/contract/renderer/drawing/BlendMode.hpp"
#include "rtype/contract/renderer/math/Rect.hpp"
#include "rtype/contract/renderer/particle/Particle.hpp"
#include "rtype/contract/renderer/particle/ParticleSystemDesc.hpp"

namespace rtype::plugins::particle {

/// @brief An effect: how its particles behave (desc), what a simulation runs, and how they look, given with each draw.
/// Built in C++ and added to ParticleEffectAssets, or read from a file by an IParticleEffectLoader.
struct ParticleEffect {
    contract::renderer::ParticleSystemDesc desc;  ///< Spawn, initial values, forces and curves.
    core::asset::AssetId texture;                 ///< From TextureAssets::load(), or an atlas's texture.
    contract::renderer::math::Rect source;        ///< Part of the texture to show; the whole texture if empty.
    contract::renderer::BlendMode blend{contract::renderer::BlendMode::kAdditive};  ///< How particles mix.
    contract::renderer::ParticleAlignment alignment{contract::renderer::ParticleAlignment::kFixed};  ///< Quad turning.
    float stretch{0.0F};  ///< kVelocity: the quad's length grows by speed * stretch (seconds), so 0 only turns it.
    contract::renderer::ParticleFlipbook flipbook;  ///< Frames of source played over each particle's life.
    core::asset::AssetId material;  ///< From MaterialAssets; the default material if empty or not created yet.
};

}  // namespace rtype::plugins::particle
