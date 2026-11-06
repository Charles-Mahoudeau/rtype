/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Parent
*/

#pragma once

#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::ecs {
/// @brief This entity's parent. Set it with `commands.entity(child).add(Parent{.entity = parent})` or
/// World::setParent(), remove it with `commands.entity(child).remove<Parent>()` or World::removeParent(), never with
/// Components::add() or remove(): the World keeps the parent's Children in sync.
struct Parent {
    Entity entity;  ///< The parent.
};
RTYPE_ECS_COMPONENT(Parent, "rtype.Parent", entity);

/// @brief The options Parent is always registered with, whoever does it first: Transforms are relative, so a client
/// needs the hierarchy to place what the server sends.
inline constexpr ComponentOptions kParentOptions{.presence = Presence::kBoth,
                                                 .replication = ReplicationMode::kEveryChange};
}  // namespace rtype::ecs
