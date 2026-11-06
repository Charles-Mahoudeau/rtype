/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** shape.vert
*/

// The vertex stage of shapes: a ShapeInstance per instance, a quad of 6 vertices built from gl_VertexIndex covering the
// shape's bounding box plus a margin of a pixel and a half for the antialiased edge, turned around its center and seen
// through the frame's camera. UV is the position in the box, 0 to 1 (a little past them in the margin). COLOR is the
// fill colour. With no game code, it is the default shader of shapes.
layout(location = 0) in vec2 rtypeCenter;            // World point the shape is centred on.
layout(location = 1) in vec2 rtypeHalfSize;          // Half its bounding box, before rotation.
layout(location = 2) in float rtypeRotation;         // Radians, clockwise on screen.
layout(location = 3) in float rtypeRadius;           // Circle / capsule radius, or corner radius.
layout(location = 4) in float rtypeOutlineThickness;
layout(location = 5) in uint rtypeKindAndBlend;      // Low byte: 0 rect, 1 circle, 2 capsule. Next byte: blend mode.
layout(location = 6) in vec4 rtypeFillColor;
layout(location = 7) in vec4 rtypeOutlineColor;
#ifdef RTYPE_INSTANCE_VALUES
layout(location = 8) in vec4 rtypeInstance[4];  // The drawing's instance values.
#endif
layout(location = 0) out vec2 rtypeUvOut;
layout(location = 1) out vec4 rtypeColorOut;  // Premultiplied.
layout(location = 2) flat out uint rtypeBlendOut;
#ifdef RTYPE_INSTANCE_VALUES
layout(location = 3) flat out vec4 rtypeInstanceOut[4];
#endif
layout(location = 7) out vec2 rtypeLocalOut;  // Position in the shape's frame, in world units.
layout(location = 8) flat out vec4 rtypeBoundsOut;  // Half size, radius, outline thickness.
layout(location = 9) flat out uint rtypeKindOut;
layout(location = 10) flat out vec4 rtypeOutlineColorOut;  // Premultiplied.

const vec2 rtypeCorners[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                                    vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

#pragma rtype_shader

void main() {
    // The size of a pixel in world units, from the camera's scale: the margin keeps the antialiased edge inside the
    // quad whatever the zoom.
    const vec2 rtypePixel = 2.0 / (abs(vec2(rtypeFrame.viewProjection[0][0], rtypeFrame.viewProjection[1][1])) *
                                   rtypeFrame.framebufferSize);
    const vec2 rtypeLocal = rtypeCorners[gl_VertexIndex] * (rtypeHalfSize + (1.5 * max(rtypePixel.x, rtypePixel.y)));
    const float rtypeCos = cos(rtypeRotation);
    const float rtypeSin = sin(rtypeRotation);
    // With Y down, this standard rotation turns clockwise on screen.
    VERTEX = rtypeCenter + (mat2(rtypeCos, rtypeSin, -rtypeSin, rtypeCos) * rtypeLocal);
    UV = (rtypeLocal / max(2.0 * rtypeHalfSize, vec2(1e-5))) + 0.5;
    COLOR = rtypeFillColor;
#pragma rtype_main
    gl_Position = rtypeFrame.viewProjection * vec4(VERTEX, 0.0, 1.0);
    rtypeUvOut = UV;
    rtypeColorOut = vec4(COLOR.rgb * COLOR.a, COLOR.a);
    rtypeBlendOut = (rtypeKindAndBlend >> 8u) & 0xFFu;
#ifdef RTYPE_INSTANCE_VALUES
    rtypeInstanceOut = rtypeInstance;
#endif
    rtypeLocalOut = rtypeLocal;
    rtypeBoundsOut = vec4(rtypeHalfSize, rtypeRadius, rtypeOutlineThickness);
    rtypeKindOut = rtypeKindAndBlend & 0xFFu;
    rtypeOutlineColorOut = vec4(rtypeOutlineColor.rgb * rtypeOutlineColor.a, rtypeOutlineColor.a);
}
