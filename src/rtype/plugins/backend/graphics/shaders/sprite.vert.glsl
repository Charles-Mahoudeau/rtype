/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** sprite.vert
*/

// The vertex stage of sprites and glyphs: a SpriteInstance per instance, a unit quad of 6 vertices built from
// gl_VertexIndex, scaled, turned around its origin and seen through the frame's camera. Without a shader, the default
// shader of sprites and glyphs.
layout(location = 0) in vec2 rtypePosition;   // World point the origin sits on.
layout(location = 1) in vec2 rtypeSize;       // In world units.
layout(location = 2) in vec2 rtypeOrigin;     // Pivot from the top-left corner, in world units.
layout(location = 3) in float rtypeRotation;  // Radians, clockwise on screen.
layout(location = 4) in vec4 rtypeUvRect;     // (u0, v0, u1, v1).
layout(location = 5) in vec4 rtypeColor;      // Multiplies the texture.
layout(location = 6) in uint rtypeBlend;      // The blend mode, and the MSDF distance range of a glyph.
#ifdef RTYPE_INSTANCE_VALUES
layout(location = 7) in vec4 rtypeInstance[4];  // The drawing's instance values.
#endif
layout(location = 0) out vec2 rtypeUvOut;
layout(location = 1) out vec4 rtypeColorOut;
layout(location = 2) flat out uint rtypeBlendOut;
#ifdef RTYPE_INSTANCE_VALUES
layout(location = 3) flat out vec4 rtypeInstanceOut[4];
#endif

const vec2 rtypeCorners[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                                    vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

#pragma rtype_shader

void main() {
    const vec2 rtypeCorner = rtypeCorners[gl_VertexIndex];
    const float rtypeCos = cos(rtypeRotation);
    const float rtypeSin = sin(rtypeRotation);
    // With Y down, this standard rotation turns clockwise on screen.
    VERTEX = rtypePosition + (mat2(rtypeCos, rtypeSin, -rtypeSin, rtypeCos) * ((rtypeCorner * rtypeSize) - rtypeOrigin));
    UV = mix(rtypeUvRect.xy, rtypeUvRect.zw, rtypeCorner);
    COLOR = rtypeColor;
#pragma rtype_main
    gl_Position = rtypeFrame.viewProjection * vec4(VERTEX, 0.0, 1.0);
    rtypeUvOut = UV;
    rtypeColorOut = vec4(COLOR.rgb * COLOR.a, COLOR.a);
    rtypeBlendOut = rtypeBlend;
#ifdef RTYPE_INSTANCE_VALUES
    rtypeInstanceOut = rtypeInstance;
#endif
}
