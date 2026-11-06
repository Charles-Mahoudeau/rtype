/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PrefabSpawner
*/

#pragma once

#include <concepts>
#include <cstddef>
#include <span>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/prefab/ComponentValues.hpp"
#include "rtype/ecs/prefab/Prefab.hpp"
#include "rtype/ecs/prefab/Prefabs.hpp"

namespace rtype::ecs::detail {
/// @brief What spawning a prefab does with each step: Commands records it, the World applies it at once.
template <typename Target>
concept PrefabTarget = requires(Target& target, Entity entity, ComponentId id, std::span<const std::byte> value) {
    { target.spawnEntity() } -> std::same_as<Entity>;
    target.addValue(entity, id, value);
    target.setParent(entity, entity);
};

/// @brief Spawns a prefab and its children, depth first: the entity, its kept values, then each child (its own tree,
/// its overrides, then its Parent).
/// @details The values a spawn does not keep on the registry's side (Prefabs::keeps()) are left out, overrides
/// included. Both ways of spawning go through it, so they create the same entities in the same order.
/// @return The prefab's entity: the root of the tree.
template <PrefabTarget Target>
Entity spawnPrefabTree(const Prefabs& prefabs, const ComponentRegistry& registry, const Prefab& prefab, Target& target);
}  // namespace rtype::ecs::detail

#define RTYPE_PREFAB_SPAWNER_INCLUDING_TPP
#include "PrefabSpawner.tpp"  // IWYU pragma: export
#undef RTYPE_PREFAB_SPAWNER_INCLUDING_TPP
