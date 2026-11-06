/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientCalls
*/

#include "rtype/replication/client/ClientCalls.hpp"

#include <cstdint>
#include <optional>
#include <string>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/replication/client/ClientState.hpp"

namespace rtype::replication::client {
namespace {
/// @brief `rtype.replication.localClient`: this client's id, once the server gave it.
LocalClient localClient(ecs::Res<ClientState> state) {
    const std::optional<network::ClientId> id = state->clientId();
    return LocalClient{.connected = id.has_value(), .client = id.value_or(std::uint32_t{0})};
}
}  // namespace

void addClientCalls(ecs::App& app) { app.addCall(std::string{kLocalClientCall}, &localClient); }
}  // namespace rtype::replication::client
