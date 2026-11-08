/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminClient
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/io/UdpSocket.hpp"

namespace rtype::network::admin {
/// @brief The tool end of the admin channel: sends requests to a server on this machine and collects its replies.
///
/// @details Requests are datagrams and may be lost: a caller that needs an answer asks again after a delay. Each
/// request gets an id, and its reply carries it. Replies from anyone but the server's admin port are ignored. Not
/// thread-safe.
class RTYPE_NETWORK_API AdminClient {
  public:
    /// @brief Opens a socket on any local port.
    /// @param port The admin port of the server, on `127.0.0.1`.
    /// @param token The secret of the server.
    /// @throws exceptions::SocketException if no socket can be opened.
    AdminClient(std::uint16_t port, std::string token);

    /// @brief Sends a request.
    /// @return Its id, or 0 when it could not be encoded.
    std::uint32_t send(const Request& request);

    /// @brief Collects the replies that arrived since the last call. Never blocks.
    [[nodiscard]] std::vector<DecodedReply> poll();

  private:
    io::UdpSocket _socket;     ///< Any local port.
    io::Address _server;       ///< `127.0.0.1:port`.
    std::string _token;        ///< The secret.
    std::uint32_t _nextId{1};  ///< The id of the next request.
};
}  // namespace rtype::network::admin
