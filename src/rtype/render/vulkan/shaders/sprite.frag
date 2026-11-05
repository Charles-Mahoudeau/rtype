/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** sprite
*/

// The default effect of sprites and particles, fragment stage: the shaders ABI (plugins/backend/graphics/shaders/)
// without an effect, the stage game effects are assembled in for kSprite and kParticle.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"
#include "fragment_common.glsl"
#include "instance_input.glsl"
#include "sprite.frag.glsl"
