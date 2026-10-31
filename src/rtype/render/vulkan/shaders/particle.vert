/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** particle
*/

// Draws one particle per instance (graphics::ParticleInstance, read as per-instance attributes from the CPU instances
// of a frame or, later, the live particles a GPU system packed): a unit quad of 6 vertices, built from gl_VertexIndex,
// centred on the particle, scaled, turned (along its velocity for kParticleVelocity, stretched by its speed) and seen
// through the frame's camera, showing the flipbook frame of its life. Its outputs are the ones of sprite.vert, so
// sprite.frag shades particles as it shades sprites.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "FrameUniforms.glsl"
#include "ParticleConstants.glsl"

layout(location = 0) in vec4 inColor;      // Multiplies the texture: tint and transparency.
layout(location = 1) in vec2 inPosition;   // Centre of the particle, in world units.
layout(location = 2) in vec2 inSize;       // In world units.
layout(location = 3) in float inRotation;  // Radians, clockwise on screen.
layout(location = 4) in vec2 inVelocity;   // World units per second: turns and stretches kParticleVelocity quads.
layout(location = 5) in float inLife;      // 0 at birth, 1 at death: picks the flipbook frame.

layout(location = 0) out vec2 outUv;
layout(location = 1) out vec4 outColor;  // Premultiplied.
layout(location = 2) flat out uint outBlend;

const vec2 kCorners[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                                vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main() {
    const vec2 corner = kCorners[gl_VertexIndex];
    float rotation = inRotation;
    vec2 size = inSize;
    const float speed = length(inVelocity);
    if (particles.alignment == kParticleVelocity && speed > 0.0) {
        // The quad's +X along the velocity, longer the faster it goes.
        rotation = atan(inVelocity.y, inVelocity.x);
        size.x += speed * particles.stretch;
    }
    const float c = cos(rotation);
    const float s = sin(rotation);
    // With Y down, this standard rotation turns clockwise on screen, around the particle's centre.
    const vec2 local = mat2(c, s, -s, c) * ((corner - 0.5) * size);
    gl_Position = frame.viewProjection * vec4(inPosition + local, 0.0, 1.0);
    // The flipbook frame of this moment of the life, a cell of the uv rect: left to right, then top to bottom.
    const uint frame = uint(clamp(inLife, 0.0, 0.9999) * particles.cycles * float(particles.frameCount)) %
                       particles.frameCount;
    const vec2 cellSize = (particles.uvRect.zw - particles.uvRect.xy) / vec2(particles.columns, particles.rows);
    const vec2 cellMin = particles.uvRect.xy + (vec2(frame % particles.columns, frame / particles.columns) * cellSize);
    outUv = cellMin + (corner * cellSize);
    outColor = vec4(inColor.rgb * inColor.a, inColor.a);
    outBlend = particles.blend;
}
