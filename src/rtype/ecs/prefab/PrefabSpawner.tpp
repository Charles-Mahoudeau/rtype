/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PrefabSpawner
*/

#pragma once

#ifndef RTYPE_PREFAB_SPAWNER_INCLUDING_TPP
#error "PrefabSpawner.tpp is an implementation detail of PrefabSpawner.hpp: include PrefabSpawner.hpp instead"
#endif

#include <cstddef>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/prefab/ComponentValues.hpp"
#include "rtype/ecs/prefab/Prefab.hpp"
#include "rtype/ecs/prefab/Prefabs.hpp"

namespace rtype::ecs::detail {
/// @brief Gives `entity` the values of `values` a spawn keeps on the registry's side, in their order.
template <PrefabTarget Target>
void addKeptValues(const Prefabs& prefabs, const ComponentRegistry& registry, const ComponentValues& values,
                   Entity entity, Target& target) {
    for (std::size_t index = 0; index < values.getCount(); ++index) {
        const ComponentInfo* info = registry.findById(values.getId(index));
        if (info != nullptr && prefabs.keeps(info->type)) {
            target.addValue(entity, info->type.id, values.getValue(index));
        }
    }
}

template <PrefabTarget Target>
// NOLINTNEXTLINE(misc-no-recursion): the depth is the depth of the prefab tree, which Prefabs keeps acyclic.
Entity spawnPrefabTree(const Prefabs& prefabs, const ComponentRegistry& registry, const Prefab& prefab,
                       Target& target) {
    const Entity entity = target.spawnEntity();
    addKeptValues(prefabs, registry, prefab.getValues(), entity, target);
    for (const PrefabChild& child : prefab.getChildren()) {
        const Entity spawned = spawnPrefabTree(prefabs, registry, *prefabs.findById(child.prefab), target);
        addKeptValues(prefabs, registry, child.overrides, spawned, target);
        target.setParent(spawned, entity);
    }
    return entity;
}
}  // namespace rtype::ecs::detail
