/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** sprite
*/

// Colours a sprite: its texture (premultiplied by the renderer when it is created), multiplied by its colour
// (premultiplied by sprite.vert), finished for its blend mode.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "Blend.glsl"
#include "ShaderLayout.glsl"

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec4 inColor;
layout(location = 2) flat in uint inBlend;

layout(location = 0) out vec4 outColor;

void main() { outColor = finishBlend(texture(uTexture, inUv) * inColor, inBlend); }
