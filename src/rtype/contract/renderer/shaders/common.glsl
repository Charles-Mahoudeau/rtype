/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** common
*/

// The shaders ABI of the renderer contract: game shaders are assembled in it (plugins/shader), and backends write their
// built-in shaders against it. The start of every stage of every shader: the frame (set 0, binding 0), the drawing's
// texture (set 1, binding 0) and the built-ins a shader reads and writes. Every name of the engine starts with rtype,
// a prefix shaders cannot use. Kept ASCII: it is embedded as chars. It has no #version: the assembler writes it first,
// as a backend's built-in shaders (the default shader, which #include these files) do.

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
