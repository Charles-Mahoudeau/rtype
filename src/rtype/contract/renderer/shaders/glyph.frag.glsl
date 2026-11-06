/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** glyph.frag
*/

// The fragment stage of glyphs: the texture read as the three distances of an MSDF font atlas, the colour covering
// the pixel where their median is past the middle. With no game code, it is the default shader of
// glyphs.
float rtypeMedian(vec3 distances) {
    return max(min(distances.r, distances.g), min(max(distances.r, distances.g), distances.b));
}

// How much of the pixel the glyph covers, 0 to 1: the median distance, scaled from atlas pixels to screen pixels so
// the edge stays about one screen pixel wide at any size. `uvWidth` is fwidth(UV); the distance range is in the
// blend word's bits 16 to 31, in 1/256 of an atlas pixel.
float rtypeCoverage(vec2 uvWidth) {
    const float distanceRange = float(rtypeBlendIn >> 16u) / 256.0;
    const vec2 unitRange = vec2(distanceRange) / vec2(textureSize(TEXTURE, 0));
    const float screenRange = max(0.5 * dot(unitRange, vec2(1.0) / uvWidth), 1.0);
    const float distance = rtypeMedian(texture(TEXTURE, UV).rgb) - 0.5;
    return clamp((distance * screenRange) + 0.5, 0.0, 1.0);
}

#pragma rtype_shader

void main() {
    UV = rtypeUvIn;
    // Out of any branch: derivatives are only defined in uniform control flow.
    const vec2 rtypeUvWidth = fwidth(UV);
    COLOR = rtypeColorIn * rtypeCoverage(rtypeUvWidth);
#pragma rtype_main
    rtypeFragColor = rtypeFinishBlend(COLOR, rtypeBlendIn & rtypeBlendMask);
}
