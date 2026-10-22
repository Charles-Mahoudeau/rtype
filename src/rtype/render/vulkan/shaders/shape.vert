/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** shape
*/

// Covers one shape per instance (rendering/ShapeInstance.hpp) with a quad of 6 vertices, built from gl_VertexIndex:
// the shape's bounding box plus a margin of a pixel and a half for the antialiased edge, turned around its center and
// seen through the frame's camera. shape.frag gets the position in the shape's own frame.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "ShaderLayout.glsl"

layout(location = 0) in vec2 inCenter;           // World point the shape is centerd on.
layout(location = 1) in vec2 inHalfSize;         // Half its bounding box, before rotation.
layout(location = 2) in float inRotation;        // Radians, clockwise on screen.
layout(location = 3) in float inRadius;          // Circle / capsule radius, or corner radius.
layout(location = 4) in float inOutlineThickness;
layout(location = 5) in uint inKind;             // 0 rect, 1 circle, 2 capsule.
layout(location = 6) in vec4 inFillColor;
layout(location = 7) in vec4 inOutlineColor;

layout(location = 0) out vec2 outLocal;  // Position in the shape's frame, in world units.
layout(location = 1) flat out vec2 outHalfSize;
layout(location = 2) flat out float outRadius;
layout(location = 3) flat out float outOutlineThickness;
layout(location = 4) flat out uint outKind;
layout(location = 5) flat out vec4 outFillColor;
layout(location = 6) flat out vec4 outOutlineColor;

const vec2 kCorners[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                                vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main() {
    // The size of a pixel in world units, from the camera's scale: the margin keeps the antialiased edge inside the
    // quad whatever the zoom.
    const vec2 pixel = 2.0 / (abs(vec2(frame.viewProjection[0][0], frame.viewProjection[1][1])) * frame.framebufferSize);
    const vec2 local = kCorners[gl_VertexIndex] * (inHalfSize + 1.5 * max(pixel.x, pixel.y));
    const float c = cos(inRotation);
    const float s = sin(inRotation);
    // With Y down, this standard rotation turns clockwise on screen.
    gl_Position = frame.viewProjection * vec4(inCenter + mat2(c, s, -s, c) * local, 0.0, 1.0);
    outLocal = local;
    outHalfSize = inHalfSize;
    outRadius = inRadius;
    outOutlineThickness = inOutlineThickness;
    outKind = inKind;
    outFillColor = inFillColor;
    outOutlineColor = inOutlineColor;
}
