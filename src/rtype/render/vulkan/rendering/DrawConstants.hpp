/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DrawConstants
*/

#pragma once

#include <cstddef>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float4.hpp>

#include "ShaderLayout.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief What changes with every draw, sent as push constants: the CPU side of the push constant block of
/// shaders/include/DrawConstants.glsl, which must match it field by field (std430 layout). Change both files together.
///
/// @details A shape is a unit quad, (0, 0) to (1, 1), that `model` places in the world (position, size, rotation,
/// origin); its texture is read from `uvRect` and multiplied by `color`. 96 bytes, below the 128 every device supports.
struct DrawConstants {
    glm::mat4 model{1.0F};                     ///< Unit quad to world units. Offset 0.
    glm::vec4 color{1.0F};                     ///< Multiplies the texture: tint and transparency. Offset 64.
    glm::vec4 uvRect{0.0F, 0.0F, 1.0F, 1.0F};  ///< Part of the texture: (u0, v0, u1, v1), 0 to 1. Offset 80.
};
static_assert(offsetof(DrawConstants, color) == 64, "DrawConstants::color must be at offset 64 (std430)");
static_assert(offsetof(DrawConstants, uvRect) == 80, "DrawConstants::uvRect must be at offset 80 (std430)");
static_assert(sizeof(DrawConstants) <= kMaxPushConstantsSize,
              "DrawConstants must fit in the push constant bytes every device supports");

}  // namespace rtype::render::vulkan::rendering
