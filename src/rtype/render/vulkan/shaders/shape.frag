/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** shape
*/

// Colours a shape from its signed distance (negative inside): the fill inside the outline band, the outline in the band
// [-thickness, 0], both antialiased over a pixel. The shape's kinds: 0 rounded box, 1 circle, 2 capsule along Y.
#version 450

layout(location = 0) in vec2 inLocal;
layout(location = 1) flat in vec2 inHalfSize;
layout(location = 2) flat in float inRadius;
layout(location = 3) flat in float inOutlineThickness;
layout(location = 4) flat in uint inKind;
layout(location = 5) flat in vec4 inFillColor;
layout(location = 6) flat in vec4 inOutlineColor;

layout(location = 0) out vec4 outColor;

// A box of half size b, its corners rounded by r.
float roundedBox(vec2 p, vec2 b, float r) {
    const vec2 q = abs(p) - (b - r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

// A segment along Y from -h to h, r thick on each side.
float capsule(vec2 p, float h, float r) {
    p.y -= clamp(p.y, -h, h);
    return length(p) - r;
}

float signedDistance() {
    if (inKind == 1u) {
        return length(inLocal) - inRadius;
    }
    if (inKind == 2u) {
        return capsule(inLocal, max(inHalfSize.y - inRadius, 0.0), inRadius);
    }
    return roundedBox(inLocal, inHalfSize, inRadius);
}

void main() {
    const float distance = signedDistance();
    const float pixel = max(fwidth(distance), 1e-5);
    // How much of the pixel is inside the shape, and inside its inner edge (where the fill starts).
    const float shape = clamp(0.5 - distance / pixel, 0.0, 1.0);
    const float fill = inOutlineThickness > 0.0 ? clamp(0.5 - (distance + inOutlineThickness) / pixel, 0.0, 1.0)
                                                : shape;
    const float outline = shape - fill;
    // Mixed with premultiplied alpha, then straightened for the pipeline's alpha blending.
    // No discard outside the shape: a transparent pixel blends to nothing, and discard would need the
    // shaderDemoteToHelperInvocation feature under Vulkan 1.3's SPIR-V.
    const float alpha = inFillColor.a * fill + inOutlineColor.a * outline;
    const vec3 premultiplied = inFillColor.rgb * inFillColor.a * fill + inOutlineColor.rgb * inOutlineColor.a * outline;
    outColor = alpha > 0.0 ? vec4(premultiplied / alpha, alpha) : vec4(0.0);
}
