/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Owner
*/

#pragma once

#include <cstdint>

#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::replication::component {
/// @brief The client that controls an entity.
/// @details Set by the game on the server, and replicated on every change so that every client knows who controls
/// what: the server applies each client's inputs to the entity it owns, and each client finds its own ship with it.
struct Owner {
    std::uint32_t client{0};  ///< The controlling client (a network::ClientId).
};

RTYPE_ECS_COMPONENT(Owner, "net.Owner", client);
}  // namespace rtype::replication::component
