/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSpawn
*/

#pragma once

#include <span>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::plugins::capsule {
class Capsules;
struct LoadedCapsule;

/// @brief Records spawning a capsule: one entity per node of this side, each a copy of its values (the server-only or
/// client-only ones left out on the other side), parented to its parent node's entity; a CapsuleInstance on the root.
/// @details The entities are reserved at once; their components are added when the commands are applied. A system
/// spawns with its Commands and a read of the Capsules resource:
/// @code
/// void spawnWave(rtype::ecs::Commands& commands, rtype::ecs::Res<rtype::plugins::capsule::Capsules> capsules) {
///     rtype::plugins::capsule::spawnCapsule(commands, *capsules, bydoId)
///         .patch<Transform>([](Transform& transform) { transform.position = {820.F, 140.F}; });
/// }
/// @endcode
/// @return The root's commands, to add more components to it or patch the capsule's.
/// @throws exceptions::CapsuleException When no capsule has this id, or one it instances is missing or contains itself;
/// nothing is recorded then.
RTYPE_PLUGIN_CAPSULE_API ecs::Commands::EntityCommands spawnCapsule(ecs::Commands& commands, const Capsules& capsules,
                                                                    CapsuleId id);

/// @brief Records switching to a capsule: despawns the root `loaded` names (the level being played, if any and alive),
/// spawns the capsule (spawnCapsule()) and makes `loaded` name it. Entities outside the old capsule's tree stay.
/// @return The new root's commands.
/// @throws exceptions::CapsuleException As spawnCapsule(); nothing is recorded and `loaded` is unchanged then.
RTYPE_PLUGIN_CAPSULE_API ecs::Commands::EntityCommands loadCapsule(ecs::Commands& commands, const Capsules& capsules,
                                                                   LoadedCapsule& loaded, CapsuleId id);

/// @brief Records respawning a capsule from its current version into the same root entity (after Capsules::define()):
/// despawns the root's children, adds the capsule's root values again (replacing those it has), spawns the children
/// again, and updates its CapsuleInstance. The root's other components stay; children the game parented to the root
/// go too.
/// @details A nested root is reloaded from its own capsule: the outer node's values on it are replaced, so the hot
/// reload reloads a nested root through the outermost capsule it belongs to.
/// @param children The root's children now, despawned first.
/// @throws exceptions::CapsuleException When the capsule cannot be spawned; nothing is recorded then.
RTYPE_PLUGIN_CAPSULE_API void reloadCapsule(ecs::Commands& commands, const Capsules& capsules, ecs::Entity root,
                                            const CapsuleInstance& instance, std::span<const ecs::Entity> children);
}  // namespace rtype::plugins::capsule
