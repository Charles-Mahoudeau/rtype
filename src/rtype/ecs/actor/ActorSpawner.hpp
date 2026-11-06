/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorSpawner
*/

#pragma once

#include <concepts>
#include <cstddef>
#include <span>

#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/Actors.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::ecs::detail {
/// @brief What spawning an actor does with each step: Commands records it, the World applies it at once.
template <typename Target>
concept ActorTarget = requires(Target& target, Entity entity, ComponentId id, std::span<const std::byte> value) {
    { target.spawnEntity() } -> std::same_as<Entity>;
    target.addValue(entity, id, value);
    target.setParent(entity, entity);
};

/// @brief Spawns an actor and its children, depth first: the entity, its kept values, then each child (its own tree,
/// its overrides, then its Parent).
/// @details The values a spawn does not keep on the registry's side (Actors::keeps()) are left out, overrides
/// included. Both ways of spawning go through it, so they create the same entities in the same order.
/// @return The actor's entity: the root of the tree.
template <ActorTarget Target>
Entity spawnActorTree(const Actors& actors, const ComponentRegistry& registry, const Actor& actor, Target& target);
}  // namespace rtype::ecs::detail

#define RTYPE_ACTOR_SPAWNER_INCLUDING_TPP
#include "ActorSpawner.tpp"  // IWYU pragma: export
#undef RTYPE_ACTOR_SPAWNER_INCLUDING_TPP
