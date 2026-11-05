/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** fragment_common
*/

// The start of every fragment stage: its inputs and output, and the blend modes (the premultiplied-alpha blend).
layout(location = 0) in vec2 rtypeUvIn;
layout(location = 1) in vec4 rtypeColorIn;  // Premultiplied.
layout(location = 2) flat in uint rtypeBlendIn;
layout(location = 0) out vec4 rtypeFragColor;

const uint rtypeBlendAdditive = 1u;
const uint rtypeBlendCutout = 2u;
const uint rtypeBlendMask = 0xFFu;

// What the premultiplied-alpha blend needs to draw `color` (premultiplied) with `blend`.
vec4 rtypeFinishBlend(vec4 color, uint blend) {
    if (blend == rtypeBlendAdditive) {
        return vec4(color.rgb, 0.0);
    }
    if (blend == rtypeBlendCutout) {
        return color.a < 0.5 ? vec4(0.0) : vec4(color.rgb / color.a, 1.0);
    }
    return color;
}
