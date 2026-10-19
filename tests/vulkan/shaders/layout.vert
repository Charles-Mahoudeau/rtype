/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** layout
*/

// Embedded in vulkan-tests to check that the renderer's shared layout headers compile together and can all be read.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "ShaderLayout.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    const vec2 corner = vec2(gl_VertexIndex & 1, gl_VertexIndex >> 1);
    outColor = draw.color * vec4(mix(draw.uvRect.xy, draw.uvRect.zw, corner), frame.time, 1.0);
    gl_Position = draw.model * vec4(corner * frame.framebufferSize, 0.0, 1.0);
}
