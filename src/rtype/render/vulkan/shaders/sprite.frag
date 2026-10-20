/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** sprite
*/

// Colours a sprite: its texture, multiplied by its colour.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "ShaderLayout.glsl"

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() { outColor = texture(uTexture, inUv) * inColor; }
