/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleRandom
*/

// The random numbers of a particle: the GPU side of plugins/backend/graphics/ParticleRandom.hpp, which must match it
// value by value so the CPU and GPU simulations draw the same numbers. Change both files together.
//
// Use it from a shader with:
//     #extension GL_GOOGLE_include_directive : require
//     #include "ParticleRandom.glsl"

#ifndef RTYPE_PARTICLE_RANDOM_GLSL
#define RTYPE_PARTICLE_RANDOM_GLSL

// Which random number of a particle is drawn: graphics::ParticleRandomStream.
const uint kStreamLifetime = 0u;
const uint kStreamSpeed = 1u;
const uint kStreamAngle = 2u;
const uint kStreamRotation = 3u;
const uint kStreamAngularVelocity = 4u;
const uint kStreamSize = 5u;
const uint kStreamShapeX = 6u;
const uint kStreamShapeY = 7u;

// PCG RXS-M-XS hash ("Hash Functions for GPU Rendering", Jarzynski and Olano, JCGT 2020).
uint pcgHash(uint value) {
    const uint state = (value * 747796405u) + 2891336453u;
    const uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

// The random number `stream` of particle `particle` of the emitter seeded with `seed`.
uint particleRandom(uint seed, uint particle, uint stream) {
    return pcgHash(pcgHash(pcgHash(seed) ^ particle) ^ stream);
}

// particleRandom() as a float in [0, 1): its top 24 bits, exact in a float, scaled by 2^-24.
float particleRandomFloat(uint seed, uint particle, uint stream) {
    return float(particleRandom(seed, particle, stream) >> 8u) * (1.0 / 16777216.0);
}

#endif
