/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetworkId
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::replication::component {
/// @brief The identity of a replicated entity, shared by the server and every client.
/// @details An Entity is a slot in one world, so it is never sent: the server gives every replicated entity a number,
/// sends that number, and each client keeps a `NetworkId → Entity` map. Given only by the server, never changed
/// afterwards. It is not replicated as a component: snapshots carry it in each record's header, and the client adds
/// it when it creates the entity.
struct NetworkId {
    std::uint32_t value{0};  ///< The shared number; 0 means none.
};

RTYPE_ECS_COMPONENT(NetworkId, "net.NetworkId", value);
}  // namespace rtype::replication::component
