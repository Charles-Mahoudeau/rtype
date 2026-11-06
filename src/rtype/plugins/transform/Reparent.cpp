/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Reparent
*/

#include "Reparent.hpp"

#include "Transform.hpp"
#include "TransformMath.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"

namespace rtype::plugins::transform {

void detachInPlace(ecs::Commands& commands, ecs::Entity child, const GlobalTransform& global) {
    commands.entity(child).remove<ecs::Parent>().add(
        Transform{.position = global.position, .rotation = global.rotation, .scale = global.scale});
}

void attachInPlace(ecs::Commands& commands, ecs::Entity child, const GlobalTransform& global, ecs::Entity parent,
                   const GlobalTransform& parentGlobal) {
    commands.entity(child).add(ecs::Parent{.entity = parent}).add(toLocal(parentGlobal, global));
}
}  // namespace rtype::plugins::transform
