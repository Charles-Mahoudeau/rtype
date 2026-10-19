/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientEvent
*/

#pragma once

#include <optional>
#include <variant>

#include "rtype/network/Types.hpp"

namespace rtype::network::client {
/// @brief Event: the server accepted the connection; the client can now send.
struct ConnectionAccepted {
    ClientId client;  ///< The id the server gave to this client.
};

/// @brief Event: a message from the server.
struct ServerMessage {
    Channel channel;  ///< The channel the server used.
    Bytes payload;    ///< The bytes of the message, type byte included.
};

/// @brief Event: an accepted connection ended, not asked for by the application.
struct Disconnected {
    DisconnectReason reason;  ///< Why.
};

/// @brief Event: the connection could not be established.
struct ConnectFailed {
    std::optional<RejectReason> reason;  ///< Why the server refused; `nullopt`: no answer in time.
};

/// @brief What the application receives from the client core.
using ClientEvent = std::variant<ConnectionAccepted, ServerMessage, Disconnected, ConnectFailed>;
}  // namespace rtype::network::client
