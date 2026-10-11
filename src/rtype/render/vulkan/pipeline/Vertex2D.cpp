/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Vertex2D
*/

#include "Vertex2D.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

namespace rtype::render::vulkan::pipeline {

std::array<vk::VertexInputBindingDescription, 1> Vertex2D::getBindings(std::uint32_t binding) noexcept {
    return {vk::VertexInputBindingDescription{binding, sizeof(Vertex2D), vk::VertexInputRate::eVertex}};
}

std::array<vk::VertexInputAttributeDescription, 3> Vertex2D::getAttributes(std::uint32_t binding) noexcept {
    return {
        vk::VertexInputAttributeDescription{0, binding, vk::Format::eR32G32Sfloat, offsetof(Vertex2D, position)},
        vk::VertexInputAttributeDescription{1, binding, vk::Format::eR32G32Sfloat, offsetof(Vertex2D, uv)},
        vk::VertexInputAttributeDescription{2, binding, vk::Format::eR8G8B8A8Unorm, offsetof(Vertex2D, color)},
    };
}

}  // namespace rtype::render::vulkan::pipeline
