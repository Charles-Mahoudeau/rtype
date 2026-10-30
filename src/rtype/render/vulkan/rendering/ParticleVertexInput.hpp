/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleVertexInput
*/

#pragma once

#include <array>
#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::rendering {

/// @return The binding of a particle instance buffer: one graphics::ParticleInstance (48 bytes, std430) per
/// instance, the CPU instances of a frame or the live particles a GPU system packed.
[[nodiscard]] RTYPE_RENDER_VULKAN_API std::array<vk::VertexInputBindingDescription, 1> getParticleBindings(
    std::uint32_t binding = 0) noexcept;

/// @return The attributes shaders/particle.vert reads: colour (location 0), position (1), size (2), rotation (3),
/// velocity (4) and life (5): every field of a ParticleInstance.
[[nodiscard]] RTYPE_RENDER_VULKAN_API std::array<vk::VertexInputAttributeDescription, 6> getParticleAttributes(
    std::uint32_t binding = 0) noexcept;

}  // namespace rtype::render::vulkan::rendering
