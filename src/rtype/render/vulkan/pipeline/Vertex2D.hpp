/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Vertex2D
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_uint4_sized.hpp>
#include <glm/vec2.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::pipeline {

/// @brief The vertex of 2D geometry in a vertex buffer: 20 bytes, read by a vertex shader as
/// layout(location = 0) in vec2 position; layout(location = 1) in vec2 uv; layout(location = 2) in vec4 color;
/// @code
/// builder.setVertexInput(Vertex2D::getBindings(), Vertex2D::getAttributes());
/// @endcode
struct Vertex2D {
    glm::vec2 position{0.0F};  ///< In world units. Location 0.
    glm::vec2 uv{0.0F};        ///< Texture coordinates. Location 1.
    glm::u8vec4 color{255};    ///< RGBA, 0-255, read as 0-1 floats (UNORM). Location 2.

    /// @return The binding of a vertex buffer of Vertex2D: one vertex per vertex, tightly packed.
    /// @param binding The binding index the vertex buffer is bound to.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static std::array<vk::VertexInputBindingDescription, 1> getBindings(
        std::uint32_t binding = 0) noexcept;

    /// @return The attributes of Vertex2D: position (location 0), uv (1) and color (2).
    /// @param binding The binding index the vertex buffer is bound to.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static std::array<vk::VertexInputAttributeDescription, 3> getAttributes(
        std::uint32_t binding = 0) noexcept;
};

static_assert(sizeof(Vertex2D) == 20 && offsetof(Vertex2D, uv) == 8 && offsetof(Vertex2D, color) == 16,
              "Vertex2D must stay tightly packed: its attribute offsets assume it");

}  // namespace rtype::render::vulkan::pipeline
