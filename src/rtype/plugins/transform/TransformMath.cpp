/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TransformMath
*/

#include "TransformMath.hpp"

#include <cmath>
#include <glm/ext/vector_float2.hpp>

#include "Transform.hpp"

namespace rtype::plugins::transform {
namespace {
/// @return @p vector turned by @p angle radians, clockwise on screen (Y down).
[[nodiscard]] glm::vec2 rotate(glm::vec2 vector, float angle) noexcept {
    const float cos = std::cos(angle);
    const float sin = std::sin(angle);
    return glm::vec2{(vector.x * cos) - (vector.y * sin), (vector.x * sin) + (vector.y * cos)};
}

/// @return -1 if @p scale mirrors, on one axis only, which reverses the rotations under it; 1 otherwise.
[[nodiscard]] float rotationDirection(glm::vec2 scale) noexcept { return scale.x * scale.y < 0.0F ? -1.0F : 1.0F; }

/// @return @p value divided by @p scale, 0 on an axis @p scale flattens.
[[nodiscard]] glm::vec2 divide(glm::vec2 value, glm::vec2 scale) noexcept {
    return glm::vec2{scale.x != 0.0F ? value.x / scale.x : 0.0F, scale.y != 0.0F ? value.y / scale.y : 0.0F};
}
}  // namespace

GlobalTransform combine(const GlobalTransform& parent, const Transform& local) noexcept {
    return GlobalTransform{.position = parent.position + rotate(local.position * parent.scale, parent.rotation),
                           .rotation = parent.rotation + (rotationDirection(parent.scale) * local.rotation),
                           .scale = parent.scale * local.scale};
}

Transform toLocal(const GlobalTransform& parent, const GlobalTransform& world) noexcept {
    return Transform{.position = divide(rotate(world.position - parent.position, -parent.rotation), parent.scale),
                     .rotation = rotationDirection(parent.scale) * (world.rotation - parent.rotation),
                     .scale = divide(world.scale, parent.scale)};
}
}  // namespace rtype::plugins::transform
