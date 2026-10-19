// What changes with every draw, as push constants: the GPU side of rendering/DrawConstants.hpp, which must match it
// field by field (std430 layout). Change both files together.
//
// A shape is a unit quad, (0, 0) to (1, 1), that draw.model places in the world; its texture is read from
// draw.uvRect and multiplied by draw.color.
#ifndef RTYPE_DRAW_CONSTANTS_GLSL
#define RTYPE_DRAW_CONSTANTS_GLSL

layout(push_constant, std430) uniform DrawConstants {
    mat4 model;   // Unit quad to world units. Offset 0.
    vec4 color;   // Multiplies the texture: tint and transparency. Offset 64.
    vec4 uvRect;  // Part of the texture: (u0, v0, u1, v1), 0 to 1. Offset 80.
} draw;

#endif
