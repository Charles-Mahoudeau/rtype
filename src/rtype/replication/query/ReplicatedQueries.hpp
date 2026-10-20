/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReplicatedQueries
*/

#pragma once

#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/system/QueryList.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::ecs {
class World;
}  // namespace rtype::ecs

/// @brief The queries through which the server reads the game's components, which no typed Query can name: they are
/// given to a system as the QueryList of a DynamicQueries parameter, one query per replicated component, in the order
/// of listReplicatedComponents().
namespace rtype::replication::query {
/// @param world A world whose replicated components are all registered.
/// @return The replicated components, in the order the queries below follow.
[[nodiscard]] RTYPE_REPLICATION_API std::vector<ecs::ComponentId> listReplicatedComponents(const ecs::World& world);

/// @brief One query per replicated component that reads its value: what a system needs to write snapshots. The
/// system reads every replicated component, so a game system that writes one in the same phase has to be ordered
/// against it.
/// @param world A world whose replicated components are all registered.
/// @return The queries.
[[nodiscard]] RTYPE_REPLICATION_API ecs::QueryList makeReplicatedReads(const ecs::World& world);

/// @brief One query per replicated component that finds the entities holding it and no NetworkId: what a system needs
/// to give them one. It reads no value and declares no access, so it conflicts with no game system.
/// @param world A world whose replicated components and NetworkId are registered.
/// @return The queries.
/// @throws exceptions::ReplicationException When NetworkId is not registered.
[[nodiscard]] RTYPE_REPLICATION_API ecs::QueryList makeUntrackedQueries(const ecs::World& world);
}  // namespace rtype::replication::query
