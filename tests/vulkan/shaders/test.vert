/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** test
*/

// Embedded in vulkan-tests to check the glsl.spirv rule: compilation, includes and the generated header.
#version 450
#extension GL_GOOGLE_include_directive : require

#include "TestValue.glsl"

void main() { gl_Position = vec4(kTestValue, 0.0, 0.0, 1.0); }
