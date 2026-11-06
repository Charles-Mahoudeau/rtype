/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Reparent
*/

#pragma once

#include "Transform.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::transform {
/// @brief Records making @p child a root without moving it: `remove<ecs::Parent>()` alone would leave a Transform
/// relative to a parent it no longer has. @p global comes from a query, as of the last PostUpdate.
/// @code
/// rtype::plugins::transform::detachInPlace(commands, turret, global);  // the turret falls off the boss, where it was
/// @endcode
void detachInPlace(ecs::Commands& commands, ecs::Entity child, const GlobalTransform& global);

/// @brief Records making @p child a child of @p parent without moving it, its Transform becoming relative to it.
void attachInPlace(ecs::Commands& commands, ecs::Entity child, const GlobalTransform& global, ecs::Entity parent,
                   const GlobalTransform& parentGlobal);
}  // namespace rtype::plugins::transform
