/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSystem
*/

// The buffers and push constants of a GPU particle system, as its compute shaders read them: the GPU side of
// particle/GpuParticleData.hpp, which must match it field by field (std430 layout). Change both files together.
//
// Use it from a compute shader with:
//     #extension GL_GOOGLE_include_directive : require
//     #include "ParticleSystem.glsl"

#ifndef RTYPE_PARTICLE_SYSTEM_GLSL
#define RTYPE_PARTICLE_SYSTEM_GLSL

const uint kCurveSamples = 64u;  // graphics::kParticleCurveSamples.

const uint kShapePoint = 0u;
const uint kShapeCircle = 1u;
const uint kShapeRect = 2u;

const uint kSpaceWorld = 0u;  // graphics::ParticleSpace::kWorld.
const uint kSpaceLocal = 1u;  // graphics::ParticleSpace::kLocal.

// GpuParticle: what a slot keeps between frames.
struct Particle {
    vec2 position;  // In the world (kSpaceWorld), or relative to the emitter (kSpaceLocal).
    vec2 velocity;  // World units per second, before speedOverLife.
    float rotation;
    float angularVelocity;
    float size;  // At birth, before sizeOverLife.
    float age;
    float lifetime;
    float padding;
};

// graphics::ParticleInstance: a particle as it is drawn (particle.vert reads the same layout).
struct ParticleInstance {
    vec4 color;
    vec2 position;
    vec2 velocity;
    vec2 size;
    float rotation;
    float lifetime;  // Seconds left to live.
};

layout(set = 0, binding = 0, std430) readonly buffer ParticleParams {
    vec4 color;
    vec2 gravity;
    vec2 lifetime;  // (min, max), as every vec2 range below.
    vec2 speed;
    vec2 angle;
    vec2 rotation;
    vec2 angularVelocity;
    vec2 size;
    vec2 shapeSize;  // Rect: its size; circle: (radius, 0).
    float drag;
    uint shape;
    uint edgeOnly;
    uint space;
    uint capacity;
    vec4 colorOverLife[kCurveSamples];
    float sizeOverLife[kCurveSamples];
    float speedOverLife[kCurveSamples];
} params;

layout(set = 0, binding = 1, std430) buffer Particles {
    Particle slots[];
} particles;

layout(set = 0, binding = 2, std430) buffer DeadList {
    uint slots[];
} dead;

// Two lists of `capacity` live slots each: list `current` is read this frame, the other written.
layout(set = 0, binding = 3, std430) buffer AliveLists {
    uint slots[];
} alive;

layout(set = 0, binding = 4, std430) buffer Counters {
    int dead;
    uint alive[2];
} counters;

layout(set = 0, binding = 5, std430) writeonly buffer Instances {
    ParticleInstance instances[];
} drawn;

// The VkDrawIndirectCommand of the draw: vertexCount, instanceCount, firstVertex, firstInstance.
layout(set = 0, binding = 6, std430) buffer Indirect {
    uint command[4];
} indirect;

// GpuParticlePush: the frame's emission (graphics::ParticleEmission) and which alive list is read.
layout(push_constant, std430) uniform Emission {
    vec2 position;
    vec2 scale;
    float rotation;
    float deltaTime;
    uint seed;
    uint firstParticle;
    uint spawnCount;
    uint current;
} emission;

// The baked tables at t (clamped to [0, 1], NaN reading the first sample), linearly interpolated: sampleBaked() of
// graphics/ParticleCurve.hpp.
#define RTYPE_SAMPLE_TABLE(table, t)                                                    \
    ((isnan(t) || (t) <= 0.0) ? table[0]                                               \
     : ((t) >= 1.0)           ? table[kCurveSamples - 1u]                              \
                              : mix(table[uint((t) * float(kCurveSamples - 1u))],      \
                                    table[uint((t) * float(kCurveSamples - 1u)) + 1u], \
                                    fract((t) * float(kCurveSamples - 1u))))

vec4 sampleColorOverLife(float t) { return RTYPE_SAMPLE_TABLE(params.colorOverLife, t); }
float sampleSizeOverLife(float t) { return RTYPE_SAMPLE_TABLE(params.sizeOverLife, t); }
float sampleSpeedOverLife(float t) { return RTYPE_SAMPLE_TABLE(params.speedOverLife, t); }

#endif
