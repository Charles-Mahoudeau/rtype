/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CapsuleSpawn
*/

#pragma once

#include <cstddef>
#include <span>
#include <string_view>

#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsuleInstance.hpp"
#include "plugins/capsule/Export.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"

namespace rtype::ecs {
class World;
}  // namespace rtype::ecs

namespace rtype::plugins::capsule {
class Capsules;
struct LoadedCapsule;

/// @brief Records spawning a capsule: one entity per node of this side, each a copy of its values (the server-only or
/// client-only ones left out on the other side), parented to its parent node's entity; a CapsuleInstance on the root.
/// @details The entities are reserved at once; their components are added when the commands are applied. A system
/// spawns with its Commands and a read of the Capsules resource:
/// @code
/// void spawnWave(rtype::ecs::Commands& commands, rtype::ecs::Res<rtype::plugins::capsule::Capsules> capsules) {
///     rtype::plugins::capsule::spawnCapsule(commands, *capsules, "actors/bydo.actor")
///         .add(Transform{.position = {820.F, 140.F}});
/// }
/// @endcode
/// @return The root's commands, to add more components to it (they replace the capsule's).
/// @throws exceptions::CapsuleException When no capsule has this id, or one it instances is missing or contains itself;
/// nothing is recorded then.
RTYPE_PLUGIN_CAPSULE_API ecs::Commands::EntityCommands spawnCapsule(ecs::Commands& commands, const Capsules& capsules,
                                                                    CapsuleId id);

/// @brief Records spawning the capsule at `path` (spawnCapsule() by id).
/// @throws exceptions::CapsuleException As spawnCapsule() by id, or when no capsule is at `path`.
RTYPE_PLUGIN_CAPSULE_API ecs::Commands::EntityCommands spawnCapsule(ecs::Commands& commands, const Capsules& capsules,
                                                                    std::string_view path);

/// @brief Spawns a capsule on the World at once, from the world's Capsules resource. If a step fails, what it spawned
/// is despawned: no half-built tree is left behind.
/// @return The root.
/// @throws exceptions::CapsuleException When the world has no Capsules (the CapsulePlugin was not added), or as
/// spawnCapsule() on Commands.
RTYPE_PLUGIN_CAPSULE_API ecs::Entity spawnCapsule(ecs::World& world, CapsuleId id);

/// @brief Spawns the capsule at `path` on the World at once (spawnCapsule() by id).
RTYPE_PLUGIN_CAPSULE_API ecs::Entity spawnCapsule(ecs::World& world, std::string_view path);

/// @brief Records switching to a capsule: despawns the root `loaded` names (the level being played, if any and alive),
/// spawns the capsule (spawnCapsule()) and makes `loaded` name it. Entities outside the old capsule's tree stay.
/// @return The new root's commands.
/// @throws exceptions::CapsuleException As spawnCapsule(); nothing is recorded and `loaded` is unchanged then.
RTYPE_PLUGIN_CAPSULE_API ecs::Commands::EntityCommands loadCapsule(ecs::Commands& commands, const Capsules& capsules,
                                                                   LoadedCapsule& loaded, CapsuleId id);

/// @brief Records switching to the capsule at `path` (loadCapsule() by id).
RTYPE_PLUGIN_CAPSULE_API ecs::Commands::EntityCommands loadCapsule(ecs::Commands& commands, const Capsules& capsules,
                                                                   LoadedCapsule& loaded, std::string_view path);

/// @brief Switches to a capsule on the World at once, from the world's Capsules and LoadedCapsule resources
/// (loadCapsule() on Commands).
/// @return The new root.
/// @throws exceptions::CapsuleException When the world has no Capsules or LoadedCapsule, or as spawnCapsule().
RTYPE_PLUGIN_CAPSULE_API ecs::Entity loadCapsule(ecs::World& world, CapsuleId id);

/// @brief Switches to the capsule at `path` on the World at once (loadCapsule() by id).
RTYPE_PLUGIN_CAPSULE_API ecs::Entity loadCapsule(ecs::World& world, std::string_view path);

/// @brief Respawns a capsule from its current version into the same root entity (after Capsules::define()): despawns
/// the root's children, adds the capsule's root values again (replacing those it has), spawns the children again, and
/// updates its CapsuleInstance. The root's other components stay; children the game parented to the root go too.
/// @details A nested root is reloaded from its own capsule: the outer node's values on it are replaced. Use
/// reloadInstances() to reload what a capsule spawned, nested roots through their outermost capsule.
/// @throws exceptions::CapsuleException When `root` has no CapsuleInstance, the world has no Capsules, or the capsule
/// cannot be spawned (nothing changes then).
RTYPE_PLUGIN_CAPSULE_API void reloadCapsule(ecs::World& world, ecs::Entity root);

/// @brief Records the reload of a root (reloadCapsule() on the World), from a system that queries its CapsuleInstance
/// and Children.
/// @param children The root's children now, despawned first.
/// @throws exceptions::CapsuleException When the capsule cannot be spawned; nothing is recorded then.
RTYPE_PLUGIN_CAPSULE_API void reloadCapsule(ecs::Commands& commands, const Capsules& capsules, ecs::Entity root,
                                            const CapsuleInstance& instance, std::span<const ecs::Entity> children);

/// @brief Reloads everything spawned from a capsule (after it was defined again): each root of it, a nested one through
/// the outermost capsule it belongs to, each outermost root once.
/// @return The number of roots reloaded.
/// @throws exceptions::CapsuleException As reloadCapsule().
RTYPE_PLUGIN_CAPSULE_API std::size_t reloadInstances(ecs::World& world, CapsuleId id);
}  // namespace rtype::plugins::capsule
