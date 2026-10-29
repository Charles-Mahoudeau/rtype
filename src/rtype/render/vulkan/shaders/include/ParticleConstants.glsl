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
// then read particles.uvRect, particles.blend and particles.alignment.

#ifndef RTYPE_PARTICLE_CONSTANTS_GLSL
#define RTYPE_PARTICLE_CONSTANTS_GLSL

layout(push_constant, std430) uniform ParticleConstants {
    vec4 uvRect;     // Part of the texture: (u0, v0, u1, v1), 0 to 1. Offset 0.
    uint blend;      // Blend.glsl: kBlendAlpha, kBlendAdditive or kBlendCutout. Offset 16.
    uint alignment;  // graphics::ParticleAlignment: kParticleFixed or kParticleVelocity. Offset 20.
} particles;

const uint kParticleFixed = 0u;     // Drawn with its own rotation and size.
const uint kParticleVelocity = 1u;  // Turned along its velocity (not handled yet: drawn as kParticleFixed).

#endif
