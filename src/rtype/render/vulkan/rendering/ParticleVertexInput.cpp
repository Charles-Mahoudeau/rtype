/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleVertexInput
*/

#include "ParticleVertexInput.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Particle.hpp"

namespace rtype::render::vulkan::rendering {

namespace {
using plugins::backend::graphics::ParticleInstance;
}  // namespace

std::array<vk::VertexInputBindingDescription, 1> getParticleBindings(std::uint32_t binding) noexcept {
    return {vk::VertexInputBindingDescription{binding, sizeof(ParticleInstance), vk::VertexInputRate::eInstance}};
}

std::array<vk::VertexInputAttributeDescription, 4> getParticleAttributes(std::uint32_t binding) noexcept {
    return {
        vk::VertexInputAttributeDescription{0, binding, vk::Format::eR32G32B32A32Sfloat,
                                            offsetof(ParticleInstance, color)},
        vk::VertexInputAttributeDescription{1, binding, vk::Format::eR32G32Sfloat,
                                            offsetof(ParticleInstance, position)},
        vk::VertexInputAttributeDescription{2, binding, vk::Format::eR32G32Sfloat, offsetof(ParticleInstance, size)},
        vk::VertexInputAttributeDescription{3, binding, vk::Format::eR32Sfloat, offsetof(ParticleInstance, rotation)},
    };
}

}  // namespace rtype::render::vulkan::rendering
