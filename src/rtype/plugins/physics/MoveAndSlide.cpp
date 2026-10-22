/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MoveAndSlide
*/

#include "plugins/physics/MoveAndSlide.hpp"

#include <algorithm>
#include <cstdint>
#include <format>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <optional>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::physics {
namespace {
/// @brief Below this length, in world units, a move is over.
constexpr float kMinMove = 1e-4F;
/// @brief Below this cosine between the move and the surface, the shape grazes it: the skin is kept along the move.
constexpr float kMinFacing = 0.1F;
}  // namespace

glm::vec2 moveAndSlide(const PhysicsWorld& world, const Collider& shape, glm::vec2 from, glm::vec2 delta,
                       const QueryFilter& filter, const SlideSettings& settings) {
    if (!(settings.skin >= 0.0F)) {
        throw exceptions::PhysicsException{
            std::format("moveAndSlide: the skin must not be negative, got {}", settings.skin)};
    }
    glm::vec2 position = from;
    glm::vec2 remaining = delta;
    for (std::uint32_t slide = 0; slide <= settings.maxSlides; ++slide) {
        const float distance = glm::length(remaining);
        if (distance <= kMinMove) {
            break;
        }
        const glm::vec2 direction = remaining / distance;
        const std::optional<PhysicsWorld::RayHit> hit =
            world.shapeCast(shape, position, direction, distance + settings.skin, filter);
        if (!hit) {
            position += remaining;
            break;
        }
        const float facing = -glm::dot(direction, hit->normal);
        const float backOff = facing > kMinFacing ? settings.skin / facing : settings.skin;
        const float travel = std::clamp(hit->distance - backOff, 0.0F, distance);
        position += direction * travel;
        if (glm::length(hit->normal) <= kMinMove || slide == settings.maxSlides) {
            break;
        }
        const glm::vec2 left = direction * (distance - travel);
        remaining = left - (hit->normal * glm::dot(left, hit->normal));
    }
    return position;
}

glm::vec2 moveAndSlide(const PhysicsWorld& world, ecs::Entity entity, const Collider& collider, glm::vec2 from,
                       glm::vec2 delta, const SlideSettings& settings) {
    return moveAndSlide(world, collider, from, delta, QueryFilter{.mask = collider.mask, .ignore = entity}, settings);
}

}  // namespace rtype::plugins::physics
