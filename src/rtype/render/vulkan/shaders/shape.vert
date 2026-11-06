/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** shape
*/

// The default shader of shapes, vertex stage: the shaders ABI (contract/renderer/shaders/) with no game code,
// the stage game shaders are assembled in for kShape, so built-in and game shapes are drawn alike.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"
#include "shape.vert.glsl"
