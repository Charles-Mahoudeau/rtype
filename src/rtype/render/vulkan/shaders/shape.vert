/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** shape
*/

// The default effect of shapes, vertex stage: the shaders ABI (plugins/backend/graphics/shaders/) without an effect,
// the stage game effects are assembled in for kShape, so built-in and game shapes are drawn alike.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"
#include "shape.vert.glsl"
