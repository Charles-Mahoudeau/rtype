/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleEffect
*/

#pragma once

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/backend/math/Rect.hpp"

namespace rtype::plugins::particle {

/// @brief An effect: how its particles behave (desc), what a simulation runs, and how they look, given with each draw.
/// Built in C++ and added to ParticleEffectAssets, or read from a file by an IParticleEffectLoader.
struct ParticleEffect {
    backend::graphics::ParticleSystemDesc desc;  ///< Spawn, initial values, forces and curves.
    core::asset::AssetId texture;                ///< From TextureAssets::load(), or an atlas's texture.
    backend::math::Rect source;                  ///< Part of the texture to show; the whole texture if empty.
    backend::graphics::BlendMode blend{backend::graphics::BlendMode::kAdditive};  ///< How particles mix.
    backend::graphics::ParticleAlignment alignment{backend::graphics::ParticleAlignment::kFixed};  ///< Quad turning.
    float stretch{0.0F};  ///< kVelocity: the quad's length grows by speed * stretch (seconds), so 0 only turns it.
};

}  // namespace rtype::plugins::particle
