/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** mesh
*/

// Vertex stage of the meshes IRenderer::drawMesh() draws over the frame (a debug UI, an editor): triangles placed in
// framebuffer pixels. The colour comes in sRGB and not premultiplied, as the contract says, and leaves in linear
// space and premultiplied, which is what the sRGB attachment and the blending of the other passes expect.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "FrameUniforms.glsl"

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inUv;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 outColor;

vec3 srgbToLinear(vec3 color) {
    return mix(pow((color + 0.055) / 1.055, vec3(2.4)), color / 12.92, lessThanEqual(color, vec3(0.04045)));
}

void main() {
    gl_Position = frame.viewProjection * vec4(inPosition, 0.0, 1.0);
    outUv = inUv;
    outColor = vec4(srgbToLinear(inColor.rgb) * inColor.a, inColor.a);
}
