/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapeInstance
*/

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Shape.hpp"
#include "render/vulkan/Export.hpp"

namespace rtype::render::vulkan::rendering {

/// @brief The geometry a shape instance carries: what shaders/shape.frag computes the signed distance of.
enum class ShapeInstanceKind : std::uint8_t {
    kRect = 0,     ///< A rounded box: halfSize, corner radius.
    kCircle = 1,   ///< A circle of radius.
    kCapsule = 2,  ///< A capsule along local Y: radius, straight part of halfSize.y - radius on each side.
};

/// @brief One shape in the per-frame instance buffer, read by shaders/shape.vert as per-instance vertex attributes:
/// 64 bytes. The vertex shader covers the shape with a quad, centred on center and turned by rotation; the fragment
/// shader computes the kind's signed distance to fill and outline it. A line is drawn as a capsule along its segment.
struct ShapeInstance {
    glm::vec2 center{0.0F};         ///< World point the shape is centred on. Location 0.
    glm::vec2 halfSize{0.0F};       ///< Half the shape's bounding box, before rotation, in world units. Location 1.
    float rotation = 0.0F;          ///< In radians, clockwise on screen, around center. Location 2.
    float radius = 0.0F;            ///< Circle and capsule radius, or the rectangle's corner radius. Location 3.
    float outlineThickness = 0.0F;  ///< In world units, inside the shape; 0: no outline. Location 4.
    ShapeInstanceKind kind = ShapeInstanceKind::kRect;  ///< Which distance the fragment computes. Location 5, low byte.
    std::uint8_t blend = 0;        ///< kBlendAlpha or kBlendAdditive (rendering/Blend.hpp). Location 5, second byte.
    std::uint16_t padding = 0;     ///< Read with kind and blend as one uint by shape.vert: must stay zero.
    glm::vec4 fillColor{1.0F};     ///< Fully transparent: outline only. Location 6.
    glm::vec4 outlineColor{0.0F};  ///< The outline's colour. Location 7.

    /// @return The instance that draws @p shape.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static ShapeInstance fromShape(
        const plugins::backend::graphics::Shape& shape) noexcept;

    /// @return The binding of the instance buffer: one ShapeInstance per instance.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static std::array<vk::VertexInputBindingDescription, 1> getBindings(
        std::uint32_t binding = 0) noexcept;
    /// @return The attributes, locations 0 to 7.
    [[nodiscard]] RTYPE_RENDER_VULKAN_API static std::array<vk::VertexInputAttributeDescription, 8> getAttributes(
        std::uint32_t binding = 0) noexcept;
};
static_assert(sizeof(ShapeInstance) == 64 && offsetof(ShapeInstance, kind) == 28 &&
                  offsetof(ShapeInstance, blend) == 29 && offsetof(ShapeInstance, fillColor) == 32 &&
                  offsetof(ShapeInstance, outlineColor) == 48,
              "ShapeInstance must stay 64 tightly packed bytes: its attribute offsets assume it");

}  // namespace rtype::render::vulkan::rendering
