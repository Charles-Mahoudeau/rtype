/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MoveAndSlide
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>

#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/QueryFilter.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::physics {

/// @brief How moveAndSlide() moves a shape.
struct SlideSettings {
    float skin = 0.5F;             ///< The gap kept between the shape and what it touches, in world units.
    std::uint32_t maxSlides = 4U;  ///< How many surfaces one move can slide along before it stops.
};

/// @brief Moves a shape by a delta without stepping the world: it stops against what it hits and slides along it.
/// @details A kinematic move for predicted systems: built on PhysicsWorld::shapeCast(), it gives the same result on the
/// server and on a client whose bodies follow the same Transforms, so a ship replaying its inputs agrees with the
/// server. Each hit stops the shape a skin away from the surface, and the rest of the delta goes on along it; a shape
/// that starts inside a body stays where it is. The shape is swept without rotation, as by shapeCast().
/// @param world The world whose bodies block the shape.
/// @param shape The shape to move: only its shape fields are read (shape, size, radius, halfHeight, offset).
/// @param from Where the shape starts, in world units, Y down.
/// @param delta How far it would move without obstacles, in world units, Y down.
/// @param filter Which bodies block it: ignore the entity that moves, or it blocks itself.
/// @param settings The skin and the number of slides.
/// @return Where the shape ends, in world units, Y down.
/// @throws exceptions::PhysicsException When the shape is invalid (as for a Collider) or the skin is negative.
/// @code
/// transform->position = rtype::plugins::physics::moveAndSlide(*world, collider, transform->position, delta,
///                                                             {.mask = collider.mask, .ignore = ship});
/// @endcode
[[nodiscard]] glm::vec2 moveAndSlide(const PhysicsWorld& world, const Collider& shape, glm::vec2 from, glm::vec2 delta,
                                     const QueryFilter& filter = {}, const SlideSettings& settings = {});

/// @brief Moves an entity's collider by a delta, blocked by the bodies its mask collides with and never by its own.
/// @details moveAndSlide() with a filter seeing Collider::mask and ignoring the entity.
/// @code
/// transform->position = rtype::plugins::physics::moveAndSlide(*world, ship, collider, transform->position, delta);
/// @endcode
[[nodiscard]] glm::vec2 moveAndSlide(const PhysicsWorld& world, ecs::Entity entity, const Collider& collider,
                                     glm::vec2 from, glm::vec2 delta, const SlideSettings& settings = {});

}  // namespace rtype::plugins::physics
