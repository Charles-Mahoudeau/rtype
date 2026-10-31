/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleConstants
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_uint2.hpp>

#include "ShaderLayout.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief What changes with every particle draw, sent as push constants: the CPU side of the push constant block of
/// shaders/include/ParticleConstants.glsl, which must match it field by field (std430 layout). Change both files
/// together.
///
/// @details What every particle of one draw shares: the part of the texture they show, how they are blended, turned
/// and stretched, and the flipbook they play. The rest is per particle, in the instances (graphics::ParticleInstance).
/// It sits at offset 0 of the shared push constant range, which DrawConstants sizes.
struct ParticleConstants {
    glm::vec4 uvRect{0.0F, 0.0F, 1.0F, 1.0F};  ///< Part of the texture: (u0, v0, u1, v1), 0 to 1. Offset 0.
    std::uint32_t blend = 0;                   ///< graphics::BlendMode, as rendering/Blend.hpp packs it. Offset 16.
    std::uint32_t alignment = 0;               ///< graphics::ParticleAlignment. Offset 20.
    float stretch = 0.0F;                      ///< kVelocity: length added per unit of speed. Offset 24.
    std::uint32_t columns = 1;                 ///< Flipbook: frames per row of uvRect, at least 1. Offset 28.
    std::uint32_t rows = 1;                    ///< Flipbook: rows of uvRect, at least 1. Offset 32.
    std::uint32_t frameCount = 1;              ///< Flipbook: frames played, 1 to columns * rows. Offset 36.
    float cycles = 1.0F;                       ///< Flipbook: times the frames play over a life. Offset 40.

    /// @return The constants of @p particles, drawn from a texture of @p textureSize pixels: an empty source shows the
    /// whole texture. The flipbook is made safe: at least one column and row, and 1 to columns * rows frames.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static ParticleConstants fromDraw(
        const plugins::backend::graphics::ParticleDraw& particles, glm::uvec2 textureSize) noexcept;
};
static_assert(offsetof(ParticleConstants, blend) == 16, "ParticleConstants::blend must be at offset 16 (std430)");
static_assert(offsetof(ParticleConstants, alignment) == 20,
              "ParticleConstants::alignment must be at offset 20 (std430)");
static_assert(offsetof(ParticleConstants, stretch) == 24, "ParticleConstants::stretch must be at offset 24 (std430)");
static_assert(offsetof(ParticleConstants, columns) == 28 && offsetof(ParticleConstants, frameCount) == 36 &&
                  offsetof(ParticleConstants, cycles) == 40,
              "ParticleConstants' flipbook must be at offsets 28 to 40 (std430)");
static_assert(static_cast<std::uint32_t>(plugins::backend::graphics::ParticleAlignment::kFixed) == 0 &&
                  static_cast<std::uint32_t>(plugins::backend::graphics::ParticleAlignment::kVelocity) == 1,
              "graphics::ParticleAlignment must keep the values of shaders/include/ParticleConstants.glsl");
static_assert(sizeof(ParticleConstants) <= kMaxPushConstantsSize,
              "ParticleConstants must fit in the push constant bytes every device supports");

}  // namespace rtype::render::vulkan::rendering
