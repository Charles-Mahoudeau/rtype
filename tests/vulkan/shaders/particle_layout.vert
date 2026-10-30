/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** particle_layout
*/

// Embedded in vulkan-tests to check that the particle system headers compile together and can all be read and written.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "ParticleRandom.glsl"
#include "ParticleSystem.glsl"

layout(location = 0) out vec4 outColor;

void main() {
    const uint slot = dead.slots[gl_VertexIndex];
    const float t = particles.slots[slot].age / particles.slots[slot].lifetime;
    outColor = sampleColorOverLife(t) * sampleSizeOverLife(t) * sampleSpeedOverLife(t) *
               particleRandomFloat(emission.seed, emission.firstParticle, kStreamShapeX);
    gl_Position = vec4(params.gravity + emission.position, float(counters.alive[emission.current]),
                       float(alive.slots[0] + indirect.command[1]));
}
