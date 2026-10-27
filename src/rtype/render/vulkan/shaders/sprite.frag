/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** sprite
*/

// Colours a sprite: its texture (premultiplied by the renderer when it is created), multiplied by its colour
// (premultiplied by sprite.vert), finished for its blend mode. A glyph of a text (kShaderMsdf) reads its texture as the
// three distances of an MSDF font atlas instead: its colour, covered where their median is past the middle.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "Blend.glsl"
#include "ShaderLayout.glsl"

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec4 inColor;
layout(location = 2) flat in uint inBlend;

layout(location = 0) out vec4 outColor;

float median(vec3 distances) {
    return max(min(distances.r, distances.g), min(max(distances.r, distances.g), distances.b));
}

// How much of the pixel the glyph covers, 0 to 1: the median distance, scaled from atlas pixels to screen pixels so
// the edge stays about one screen pixel wide at any size. `uvWidth` is fwidth(inUv).
float msdfCoverage(vec2 uvWidth, float distanceRange) {
    const vec2 unitRange = vec2(distanceRange) / vec2(textureSize(uTexture, 0));
    const float screenRange = max(0.5 * dot(unitRange, vec2(1.0) / uvWidth), 1.0);
    const float distance = median(texture(uTexture, inUv).rgb) - 0.5;
    return clamp((distance * screenRange) + 0.5, 0.0, 1.0);
}

void main() {
    // Taken out of the branch: derivatives are only defined in uniform control flow (here it is, per instance).
    const vec2 uvWidth = fwidth(inUv);
    vec4 color;
    if ((inBlend & kShaderMsdf) != 0u) {
        color = inColor * msdfCoverage(uvWidth, float(inBlend >> kDistanceRangeShift) / kDistanceRangeUnit);
    } else {
        color = texture(uTexture, inUv) * inColor;
    }
    outColor = finishBlend(color, inBlend & kBlendMask);
}
