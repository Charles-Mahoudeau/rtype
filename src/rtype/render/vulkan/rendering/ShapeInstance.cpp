/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapeInstance
*/

#include "ShapeInstance.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "plugins/backend/graphics/Shape.hpp"

namespace rtype::render::vulkan::rendering {

ShapeInstance ShapeInstance::fromShape(const plugins::backend::graphics::Shape& shape) noexcept {
    using plugins::backend::graphics::ShapeKind;
    const float radius = std::max(shape.radius, 0.0F);
    ShapeInstance instance{
        .center = shape.position,
        .rotation = shape.rotation,
        .radius = radius,
        .outlineThickness = std::max(shape.outlineThickness, 0.0F),
        .fillColor = shape.fillColor,
        .outlineColor = shape.outlineColor,
    };
    switch (shape.kind) {
        case ShapeKind::kRect:
            instance.kind = ShapeInstanceKind::kRect;
            instance.halfSize = glm::max(shape.size * 0.5F, glm::vec2{0.0F});
            instance.radius = std::clamp(shape.cornerRadius, 0.0F, std::min(instance.halfSize.x, instance.halfSize.y));
            break;
        case ShapeKind::kCircle:
            instance.kind = ShapeInstanceKind::kCircle;
            instance.halfSize = glm::vec2{radius};
            instance.rotation = 0.0F;
            break;
        case ShapeKind::kCapsule:
            instance.kind = ShapeInstanceKind::kCapsule;
            instance.halfSize = glm::vec2{radius, std::max(shape.halfHeight, 0.0F) + radius};
            break;
        case ShapeKind::kLine: {
            const glm::vec2 segment = shape.end - shape.position;
            const float length = glm::length(segment);
            instance.kind = ShapeInstanceKind::kCapsule;
            instance.center = (shape.position + shape.end) * 0.5F;
            instance.halfSize = glm::vec2{radius, (length * 0.5F) + radius};
            instance.rotation = length > 0.0F ? std::atan2(-segment.x, segment.y) : 0.0F;
            break;
        }
    }
    return instance;
}

std::array<vk::VertexInputBindingDescription, 1> ShapeInstance::getBindings(std::uint32_t binding) noexcept {
    return {vk::VertexInputBindingDescription{binding, sizeof(ShapeInstance), vk::VertexInputRate::eInstance}};
}

std::array<vk::VertexInputAttributeDescription, 8> ShapeInstance::getAttributes(std::uint32_t binding) noexcept {
    return {
        vk::VertexInputAttributeDescription{0, binding, vk::Format::eR32G32Sfloat, offsetof(ShapeInstance, center)},
        vk::VertexInputAttributeDescription{1, binding, vk::Format::eR32G32Sfloat, offsetof(ShapeInstance, halfSize)},
        vk::VertexInputAttributeDescription{2, binding, vk::Format::eR32Sfloat, offsetof(ShapeInstance, rotation)},
        vk::VertexInputAttributeDescription{3, binding, vk::Format::eR32Sfloat, offsetof(ShapeInstance, radius)},
        vk::VertexInputAttributeDescription{4, binding, vk::Format::eR32Sfloat,
                                            offsetof(ShapeInstance, outlineThickness)},
        vk::VertexInputAttributeDescription{5, binding, vk::Format::eR32Uint, offsetof(ShapeInstance, kind)},
        vk::VertexInputAttributeDescription{6, binding, vk::Format::eR32G32B32A32Sfloat,
                                            offsetof(ShapeInstance, fillColor)},
        vk::VertexInputAttributeDescription{7, binding, vk::Format::eR32G32B32A32Sfloat,
                                            offsetof(ShapeInstance, outlineColor)},
    };
}

}  // namespace rtype::render::vulkan::rendering
