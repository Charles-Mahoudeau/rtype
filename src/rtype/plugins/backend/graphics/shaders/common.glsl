/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** common
*/

// The shaders ABI of the backend contract: game effects are assembled in it (plugins/shader), and backends write their
// built-in shaders against it. The start of every stage of every effect: the frame (set 0, binding 0), the drawing's
// texture (set 1, binding 0) and the built-ins an effect reads and writes. Every name of the engine starts with rtype,
// a prefix effects cannot use. Kept ASCII: it is embedded as chars.
#version 450

layout(set = 0, binding = 0, std140) uniform RtypeFrame {
    float time;            // Seconds since the renderer started.
    float deltaTime;       // Seconds since the previous frame.
    vec2 framebufferSize;  // In pixels.
    mat4 viewProjection;   // World to clip space: the frame's camera.
} rtypeFrame;
layout(set = 1, binding = 0) uniform sampler2D TEXTURE;
#define TIME rtypeFrame.time

vec2 VERTEX;  // Vertex stage: the vertex, in world units.
vec2 UV;      // Where the texture is read.
vec4 COLOR;   // Vertex stage: the drawing's colour; fragment stage: the pixel's, premultiplied.
