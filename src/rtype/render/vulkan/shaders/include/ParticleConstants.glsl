/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleConstants
*/

// What every particle of one draw shares, as push constants: the GPU side of rendering/ParticleConstants.hpp, which
// must match it field by field (std430 layout). Change both files together. It replaces the DrawConstants block in
// the particle shaders: a shader stage has one push constant block.
//
// Use it from a shader with:
//     #extension GL_GOOGLE_include_directive : require
//     #include "ParticleConstants.glsl"
// then read particles.uvRect, particles.blend, particles.alignment, particles.stretch and the flipbook.

#ifndef RTYPE_PARTICLE_CONSTANTS_GLSL
#define RTYPE_PARTICLE_CONSTANTS_GLSL

layout(push_constant, std430) uniform ParticleConstants {
    vec4 uvRect;     // Part of the texture: (u0, v0, u1, v1), 0 to 1. Offset 0.
    uint blend;      // Blend.glsl: kBlendAlpha, kBlendAdditive or kBlendCutout. Offset 16.
    uint alignment;  // graphics::ParticleAlignment: kParticleFixed or kParticleVelocity. Offset 20.
    float stretch;   // kParticleVelocity: length added per unit of speed, in seconds. Offset 24.
    uint columns;     // Flipbook: frames per row of uvRect, at least 1. Offset 28.
    uint rows;        // Flipbook: rows of uvRect, at least 1. Offset 32.
    uint frameCount;  // Flipbook: frames played, 1 to columns * rows. Offset 36.
    float cycles;     // Flipbook: times the frames play over a life. Offset 40.
} particles;

const uint kParticleFixed = 0u;     // Drawn with its own rotation and size.
const uint kParticleVelocity = 1u;  // Turned along its velocity, the texture's +X forward, and stretched by its speed.

#endif
