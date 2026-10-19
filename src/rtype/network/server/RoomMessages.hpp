/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RoomMessages
*/

#pragma once

#include <limits>
#include <variant>

#include "rtype/network/Types.hpp"
#include "rtype/network/io/Address.hpp"

namespace rtype::network::server {
/// @brief Event: a client finished its handshake and is now in the game.
struct ClientJoined {
    ClientId client;  ///< The client that joined.
};

/// @brief Event: a client sent a message for the game (type `0x20` or above).
struct ClientMessage {
    ClientId client;  ///< The sender.
    Channel channel;  ///< The channel the sender used.
    Bytes payload;    ///< The bytes of the message, type byte included.
};

/// @brief Event: a client that had joined is gone.
struct ClientLeft {
    ClientId client;          ///< The client that left.
    DisconnectReason reason;  ///< Why.
};

/// @brief What a room receives from the server core.
using ServerEvent = std::variant<ClientJoined, ClientMessage, ClientLeft>;

/// @brief The `client` of a `SendMessage` that targets every client in the game.
inline constexpr ClientId kAllClients = std::numeric_limits<ClientId>::max();

/// @brief Command: sends a message to one client, or to all of them with `kAllClients`.
struct SendMessage {
    ClientId client;  ///< The target, or `kAllClients`.
    Channel channel;  ///< The channel to send on.
    Bytes payload;    ///< The bytes of the message, type byte included.
};

/// @brief Command: disconnects a client.
struct KickClient {
    ClientId client;          ///< The client to remove.
    DisconnectReason reason;  ///< Why; the client is told, and the `ClientLeft` event carries it.
};

/// @brief What a room sends to the server core.
using ServerCommand = std::variant<SendMessage, KickClient>;

/// @brief A datagram ready to be written to a socket.
struct OutgoingDatagram {
    io::Address to;  ///< The destination.
    Bytes bytes;     ///< The datagram.
};
}  // namespace rtype::network::server
