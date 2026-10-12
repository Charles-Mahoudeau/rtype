/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FrameUniforms
*/

#pragma once

#include <cstddef>
#include <glm/ext/vector_float2.hpp>

namespace rtype::render::vulkan::rendering {

/// @brief What every shader can read during a frame: the CPU side of the uniform block at set 0, binding 0.
///
/// @details Copied as raw bytes into the uniform buffer, so its layout must follow std140 and match
/// shaders/include/FrameUniforms.glsl field by field: change both files together. In std140, a float is aligned on
/// 4 bytes, a vec2 on 8, a vec3, a vec4 and a mat4 on 16, and the whole block is a multiple of 16 bytes. The
/// static_asserts below check it at compile time.
struct FrameUniforms {
    float time = 0.0F;                ///< Seconds since the renderer's init(). Offset 0.
    float deltaTime = 0.0F;           ///< Seconds since the previous frame, 0 on the first one. Offset 4.
    glm::vec2 framebufferSize{0.0F};  ///< Size of the rendered images, in pixels. Offset 8.
    /// TODO: alignas(16) glm::mat4 viewProjection; at offset 16, and in the GLSL block too.
};

static_assert(offsetof(FrameUniforms, deltaTime) == 4, "FrameUniforms::deltaTime must be at offset 4 (std140)");
static_assert(offsetof(FrameUniforms, framebufferSize) == 8,
              "FrameUniforms::framebufferSize must be at offset 8 (std140)");
static_assert(sizeof(FrameUniforms) % 16 == 0, "FrameUniforms' size must be a multiple of 16 bytes (std140)");

}  // namespace rtype::render::vulkan::rendering
