/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSpawner
*/

#pragma once

#include <concepts>
#include <cstddef>
#include <span>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/ComponentValues.hpp"
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::capsule::detail {
/// @brief What spawning a capsule does with each step: Commands records it, the World applies it at once.
template <typename Target>
concept CapsuleTarget =
    requires(Target& target, ecs::Entity entity, ecs::ComponentId id, std::span<const std::byte> value) {
        { target.spawnEntity() } -> std::same_as<ecs::Entity>;
        target.addValue(entity, id, value);
        target.setParent(entity, entity);
        target.addInstance(entity, CapsuleInstance{});
    };

/// @brief Checks that a spawn of `capsule` on the registry's side can go through: every instance it spawns exists and
/// none contains itself. A spawn checks first, so it creates everything or nothing.
/// @throws exceptions::CapsuleException Naming the capsule, the node and the missing or repeated capsule.
RTYPE_PLUGIN_CAPSULE_API void checkCapsule(const Capsules& capsules, const Capsule& capsule);

/// @brief Spawns `capsule`'s nodes in order: a node of the other side is skipped with its subtree; a node is a new
/// entity, or the root its instance spawns; its kept values are added (over an instanced root's); it is parented to its
/// parent's entity. The root gets a CapsuleInstance last.
/// @param root An existing entity to spawn the root into (a reload), or the null entity for a new one.
/// @param nested Whether another capsule spawns this one as a node.
/// @return The root.
/// @pre checkCapsule() passed.
template <CapsuleTarget Target>
ecs::Entity spawnTree(const Capsules& capsules, const Capsule& capsule, Target& target, ecs::Entity root, bool nested);

/// @brief Gives `entity` the values of `values` a spawn keeps on the registry's side, in their order.
template <CapsuleTarget Target>
void addKeptValues(const Capsules& capsules, const ComponentValues& values, ecs::Entity entity, Target& target);
}  // namespace rtype::plugins::capsule::detail

#define RTYPE_CAPSULE_SPAWNER_INCLUDING_TPP
#include "CapsuleSpawner.tpp"  // IWYU pragma: export
#undef RTYPE_CAPSULE_SPAWNER_INCLUDING_TPP
