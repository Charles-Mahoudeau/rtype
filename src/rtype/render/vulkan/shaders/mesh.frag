/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** mesh
*/

// Fragment stage of the meshes: the vertex colour (premultiplied) times the texture (premultiplied too, as the
// renderer premultiplies the textures it creates).
#version 450
#extension GL_GOOGLE_include_directive : require

#include "TextureSet.glsl"

layout(location = 0) in vec2 inUv;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = inColor * texture(uTexture, inUv);
}
