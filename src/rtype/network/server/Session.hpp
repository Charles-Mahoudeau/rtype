/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Session
*/

#pragma once

#include <cstdint>

#include "rtype/network/Types.hpp"
#include "rtype/network/connection/Connection.hpp"
#include "rtype/network/io/Address.hpp"

namespace rtype::network::server {
/// @brief Where a session is between the handshake and the game.
enum class SessionState : std::uint8_t {
    kWelcoming,  ///< Accepted; the welcome messages are sent and `kReady` is awaited.
    kReady,      ///< The client answered `kReady`: the game knows it.
};

/// @brief What the server keeps for one accepted client.
struct Session {
    io::Address address;                ///< Where the client is.
    ClientId client;                    ///< The id given to the client, never reused.
    std::uint32_t sessionToken;         ///< Carried by every datagram of the client.
    std::uint64_t resumeToken;          ///< Given to the client to resume later.
    std::uint64_t challenge;            ///< The challenge value that created this session.
    SessionState state;                 ///< Welcoming or ready.
    connection::Connection connection;  ///< The transport state.
};
}  // namespace rtype::network::server
