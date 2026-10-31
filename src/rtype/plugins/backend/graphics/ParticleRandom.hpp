/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleRandom
*/

#pragma once

#include <cstdint>

namespace rtype::plugins::backend::graphics {

/// @brief PCG RXS-M-XS hash, the one recommended for GPUs by "Hash Functions for GPU Rendering" (Jarzynski and
/// Olano, JCGT 2020). particle_random.glsl implements the same function: change both together, or the CPU and GPU
/// simulations stop matching.
constexpr std::uint32_t pcgHash(std::uint32_t input) noexcept {
    const std::uint32_t state = (input * 747796405U) + 2891336453U;
    const std::uint32_t word = ((state >> ((state >> 28U) + 4U)) ^ state) * 277803737U;
    return (word >> 22U) ^ word;
}

/// @brief Which random number of a particle is drawn: each property gets its own, so they are not correlated. The
/// shaders use the same values.
enum class ParticleRandomStream : std::uint8_t {
    kLifetime,
    kSpeed,
    kAngle,
    kRotation,
    kAngularVelocity,
    kSize,
    kShapeX,
    kShapeY,
};

/// @return The random number @p stream of particle @p particle of the emitter seeded with @p seed.
constexpr std::uint32_t particleRandom(std::uint32_t seed, std::uint32_t particle,
                                       ParticleRandomStream stream) noexcept {
    return pcgHash(pcgHash(pcgHash(seed) ^ particle) ^ static_cast<std::uint32_t>(stream));
}

/// @return particleRandom() as a float in [0, 1), exact on the CPU and the GPU alike.
constexpr float particleRandomFloat(std::uint32_t seed, std::uint32_t particle, ParticleRandomStream stream) noexcept {
    return static_cast<float>(particleRandom(seed, particle, stream) >> 8U) * 0x1.0p-24F;
}

}  // namespace rtype::plugins::backend::graphics
