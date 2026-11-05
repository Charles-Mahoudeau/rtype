/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** glyph
*/

// The default shader of glyphs, fragment stage (their vertex stage is sprite.vert): the shaders ABI
// (plugins/backend/graphics/shaders/) without a shader, the stage game shaders are assembled in for kGlyph.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "common.glsl"
#include "fragment_common.glsl"
#include "instance_input.glsl"
#include "glyph.frag.glsl"
