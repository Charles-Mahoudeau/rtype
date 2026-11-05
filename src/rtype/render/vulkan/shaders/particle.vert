/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** particle
*/

// The default shader of particles, vertex stage: the shaders ABI (plugins/backend/graphics/shaders/) without a shader,
// the stage game shaders are assembled in for kParticle. Its outputs are the ones of sprite.vert, so sprite.frag shades
// particles as it shades sprites. It reads the CPU instances of a frame or the live particles a GPU system packed.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"
#include "particle_constants.glsl"
#include "particle.vert.glsl"
