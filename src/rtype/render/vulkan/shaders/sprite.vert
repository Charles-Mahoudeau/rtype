/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** sprite
*/

// Draws one sprite per instance (rendering/SpriteBatch.hpp): a unit quad of 6 vertices, built from gl_VertexIndex,
// scaled, turned around the sprite's origin and seen through the frame's camera.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "ShaderLayout.glsl"

layout(location = 0) in vec2 inPosition;  // World point the origin sits on.
layout(location = 1) in vec2 inSize;      // In world units.
layout(location = 2) in vec2 inOrigin;    // Pivot from the top-left corner, in world units.
layout(location = 3) in float inRotation; // Radians, clockwise on screen.
layout(location = 4) in vec4 inUvRect;    // (u0, v0, u1, v1).
layout(location = 5) in vec4 inColor;     // Multiplies the texture.
layout(location = 6) in uint inBlend;     // Blend.glsl: kBlendAlpha, kBlendAdditive or kBlendCutout.

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 outColor;  // Premultiplied.
layout(location = 2) flat out uint outBlend;

const vec2 kCorners[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                                vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main() {
    const vec2 corner = kCorners[gl_VertexIndex];
    const float c = cos(inRotation);
    const float s = sin(inRotation);
    // With Y down, this standard rotation turns clockwise on screen.
    const vec2 local = mat2(c, s, -s, c) * (corner * inSize - inOrigin);
    gl_Position = frame.viewProjection * vec4(inPosition + local, 0.0, 1.0);
    outUv = mix(inUvRect.xy, inUvRect.zw, corner);
    outColor = vec4(inColor.rgb * inColor.a, inColor.a);
    outBlend = inBlend;
}
