/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** particle_constants
*/

// What every particle of one draw shares, as push constants: rendering/ParticleConstants.hpp, then, for an effect that
// has some (RTYPE_INSTANCE_VALUES), the draw's instance values at offset 48.
layout(push_constant, std430) uniform RtypeParticles {
    vec4 uvRect;      // Part of the texture: (u0, v0, u1, v1).
    uint blend;       // The blend mode.
    uint alignment;   // 0: fixed, 1: along the velocity.
    float stretch;    // Velocity alignment: length added per unit of speed.
    uint columns;     // Flipbook: frames per row.
    uint rows;        // Flipbook: rows.
    uint frameCount;  // Flipbook: frames played.
    float cycles;     // Flipbook: times played over a life.
#ifdef RTYPE_INSTANCE_VALUES
    vec4 instance[4];  // The draw's instance values.
#endif
} rtypeParticles;
