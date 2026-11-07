/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSpawner
*/

#pragma once

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::capsule {
class Capsules;
}  // namespace rtype::plugins::capsule

namespace rtype::plugins::capsule::detail {
/// @brief Checks that a spawn of `capsule` on the registry's side can go through: every instance it spawns exists and
/// none contains itself. A spawn checks first, so it records everything or nothing.
/// @throws exceptions::CapsuleException Naming the capsule, the node and the missing or repeated capsule.
RTYPE_PLUGIN_CAPSULE_API void checkCapsule(const Capsules& capsules, const Capsule& capsule);

/// @brief Records the spawn of `capsule`'s nodes in order: a node of the other side is skipped with its subtree; a node
/// is a new entity, or the root its instance spawns; its kept values are added (over an instanced root's); it is
/// parented to its parent's entity. The root gets a CapsuleInstance last.
/// @param root An existing entity to spawn the root into (a reload), or the null entity for a new one.
/// @param nested Whether another capsule spawns this one as a node.
/// @return The root.
/// @pre checkCapsule() passed.
RTYPE_PLUGIN_CAPSULE_API ecs::Entity spawnTree(const Capsules& capsules, const Capsule& capsule,
                                               ecs::Commands& commands, ecs::Entity root, bool nested);
}  // namespace rtype::plugins::capsule::detail
