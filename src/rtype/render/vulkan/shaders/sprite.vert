/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** sprite
*/

// The default shader of sprites, vertex stage: the shaders ABI (plugins/backend/graphics/shaders/) with no game code,
// the stage game shaders are assembled in for kSprite, so built-in and game sprites are drawn alike.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"
#include "sprite.vert.glsl"
