/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** shape.frag
*/

// The fragment stage of shapes: the colour from the shape's signed distance (negative inside), the fill inside the
// outline band, the outline in the band [-thickness, 0], both antialiased over a pixel and premultiplied. The shape's
// kinds: 0 rounded box, 1 circle, 2 capsule along Y. Without an effect, the default effect of shapes.
layout(location = 7) in vec2 rtypeLocalIn;
layout(location = 8) flat in vec4 rtypeBoundsIn;  // Half size, radius, outline thickness.
layout(location = 9) flat in uint rtypeKindIn;
layout(location = 10) flat in vec4 rtypeOutlineColorIn;  // Premultiplied.

// A box of half size b, its corners rounded by r.
float rtypeRoundedBox(vec2 p, vec2 b, float r) {
    const vec2 q = abs(p) - (b - r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

// A segment along Y from -h to h, r thick on each side.
float rtypeCapsule(vec2 p, float h, float r) {
    p.y -= clamp(p.y, -h, h);
    return length(p) - r;
}

float rtypeSignedDistance() {
    if (rtypeKindIn == 1u) {
        return length(rtypeLocalIn) - rtypeBoundsIn.z;
    }
    if (rtypeKindIn == 2u) {
        return rtypeCapsule(rtypeLocalIn, max(rtypeBoundsIn.y - rtypeBoundsIn.z, 0.0), rtypeBoundsIn.z);
    }
    return rtypeRoundedBox(rtypeLocalIn, rtypeBoundsIn.xy, rtypeBoundsIn.z);
}

#pragma rtype_effect

void main() {
    UV = rtypeUvIn;
    const float rtypeDistance = rtypeSignedDistance();
    const float rtypePixel = max(fwidth(rtypeDistance), 1e-5);
    // How much of the pixel is inside the shape, and inside its inner edge (where the fill starts).
    const float rtypeShape = clamp(0.5 - (rtypeDistance / rtypePixel), 0.0, 1.0);
    const float rtypeFill = rtypeBoundsIn.w > 0.0
                                ? clamp(0.5 - ((rtypeDistance + rtypeBoundsIn.w) / rtypePixel), 0.0, 1.0)
                                : rtypeShape;
    // No discard outside the shape: a transparent pixel blends to nothing.
    COLOR = (rtypeColorIn * rtypeFill) + (rtypeOutlineColorIn * (rtypeShape - rtypeFill));
#pragma rtype_main
    rtypeFragColor = rtypeFinishBlend(COLOR, rtypeBlendIn & rtypeBlendMask);
}
