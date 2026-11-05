/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** particle.vert
*/

// The vertex stage of particles: a ParticleInstance per instance, a unit quad centred on it, turned (along its
// velocity for the velocity alignment, stretched by its speed), showing its flipbook frame. Without a shader, the
// default shader of particles.
layout(location = 0) in vec4 rtypeColor;      // Multiplies the texture.
layout(location = 1) in vec2 rtypePosition;   // Centre of the particle, in world units.
layout(location = 2) in vec2 rtypeSize;       // In world units.
layout(location = 3) in float rtypeRotation;  // Radians, clockwise on screen.
layout(location = 4) in vec2 rtypeVelocity;   // World units per second.
layout(location = 5) in float rtypeLife;      // 0 at birth, 1 at death.
layout(location = 0) out vec2 rtypeUvOut;
layout(location = 1) out vec4 rtypeColorOut;
layout(location = 2) flat out uint rtypeBlendOut;

const vec2 rtypeCorners[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                                    vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

#pragma rtype_shader

void main() {
    const vec2 rtypeCorner = rtypeCorners[gl_VertexIndex];
    float rtypeAngle = rtypeRotation;
    vec2 rtypeQuadSize = rtypeSize;
    const float rtypeSpeed = length(rtypeVelocity);
    if (rtypeParticles.alignment == 1u && rtypeSpeed > 0.0) {
        rtypeAngle = atan(rtypeVelocity.y, rtypeVelocity.x);
        rtypeQuadSize.x += rtypeSpeed * rtypeParticles.stretch;
    }
    const float rtypeCos = cos(rtypeAngle);
    const float rtypeSin = sin(rtypeAngle);
    VERTEX = rtypePosition + (mat2(rtypeCos, rtypeSin, -rtypeSin, rtypeCos) * ((rtypeCorner - 0.5) * rtypeQuadSize));
    // The flipbook frame of this moment of the life, a cell of the uv rect: left to right, then top to bottom.
    const uint rtypeFrameIndex = uint(clamp(rtypeLife, 0.0, 0.9999) * rtypeParticles.cycles *
                                      float(rtypeParticles.frameCount)) % rtypeParticles.frameCount;
    const vec2 rtypeCellSize = (rtypeParticles.uvRect.zw - rtypeParticles.uvRect.xy) /
                               vec2(rtypeParticles.columns, rtypeParticles.rows);
    const vec2 rtypeCellMin = rtypeParticles.uvRect.xy +
        (vec2(rtypeFrameIndex % rtypeParticles.columns, rtypeFrameIndex / rtypeParticles.columns) * rtypeCellSize);
    UV = rtypeCellMin + (rtypeCorner * rtypeCellSize);
    COLOR = rtypeColor;
#pragma rtype_main
    gl_Position = rtypeFrame.viewProjection * vec4(VERTEX, 0.0, 1.0);
    rtypeUvOut = UV;
    rtypeColorOut = vec4(COLOR.rgb * COLOR.a, COLOR.a);
    rtypeBlendOut = rtypeParticles.blend;
}
