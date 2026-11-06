/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ActorSpawner
*/

#pragma once

#ifndef RTYPE_ACTOR_SPAWNER_INCLUDING_TPP
#error "ActorSpawner.tpp is an implementation detail of ActorSpawner.hpp: include ActorSpawner.hpp instead"
#endif

#include <cstddef>

#include "rtype/ecs/actor/Actor.hpp"
#include "rtype/ecs/actor/Actors.hpp"
#include "rtype/ecs/actor/ComponentValues.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::ecs::detail {
/// @brief Gives `entity` the values of `values` a spawn keeps on the registry's side, in their order.
template <ActorTarget Target>
void addKeptValues(const Actors& actors, const ComponentRegistry& registry, const ComponentValues& values,
                   Entity entity, Target& target) {
    for (std::size_t index = 0; index < values.getCount(); ++index) {
        const ComponentInfo* info = registry.findById(values.getId(index));
        if (info != nullptr && actors.keeps(info->type)) {
            target.addValue(entity, info->type.id, values.getValue(index));
        }
    }
}

template <ActorTarget Target>
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the actor tree, which Actors keeps acyclic.
Entity spawnActorTree(const Actors& actors, const ComponentRegistry& registry, const Actor& actor, Target& target) {
    const Entity entity = target.spawnEntity();
    addKeptValues(actors, registry, actor.getValues(), entity, target);
    for (const ActorChild& child : actor.getChildren()) {
        const Entity spawned = spawnActorTree(actors, registry, *actors.findById(child.actor), target);
        addKeptValues(actors, registry, child.overrides, spawned, target);
        target.setParent(spawned, entity);
    }
    return entity;
}
}  // namespace rtype::ecs::detail
