/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientCalls
*/

#pragma once

#include <cstdint>
#include <string_view>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/replication/Export.hpp"

namespace rtype::replication::client {
inline constexpr std::string_view kLocalClientCall = "rtype.replication.localClient";  ///< ClientState::clientId().

/// @brief The result of `rtype.replication.localClient`: who this client is to the server.
struct LocalClient {
    bool connected{false};    ///< Whether the server gave this client its id: false while connecting and after.
    std::uint32_t client{0};  ///< The id, as the server's `net.Owner` and `net.ClientConnected` carry it; 0 if none.
};
RTYPE_ECS_CALL_DATA(LocalClient, "rtype.replication.LocalClient", connected, client);

/// @brief Registers the client's engine calls on @p app, for scripts, on the `net.ClientState` resource:
/// `rtype.replication.localClient`, which gives this client's id, so that a script tells its own entities (their
/// `net.Owner`) from the others'. Called by the ReplicationPlugin on the client side, before the scripts load: the
/// resource only has to exist when a script calls them, once the ClientPlugin is built.
RTYPE_REPLICATION_API void addClientCalls(ecs::App& app);
}  // namespace rtype::replication::client
