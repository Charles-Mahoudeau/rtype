/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AdminServer
*/

#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <string>

#include "rtype/network/Export.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/admin/AdminProtocol.hpp"
#include "rtype/network/io/UdpSocket.hpp"
#include "rtype/network/server/NetServer.hpp"

namespace rtype::network::admin {
/// @brief What the game side tells the admin channel; the transport does not know it.
struct GameStatus {
    std::uint32_t tick{0};         ///< The server tick of the last snapshot.
    std::uint32_t entityCount{0};  ///< Entities with a NetworkId.
};

/// @brief The server end of the admin channel: a socket on `127.0.0.1` that answers the requests of a tool.
///
/// @details Every request carries the token; a wrong one gets `kBadToken` and nothing else happens. A datagram that
/// does not decode is dropped. The status and the client list come from the `NetServer`, a kick is a `KickClient`
/// command; the requests of the simulation and world ranges go to the extension handler. Not thread-safe: polled by
/// the thread that polls the `NetServer`.
class RTYPE_NETWORK_API AdminServer {
  public:
    /// @brief Answers an extension request.
    /// @return The reply, or nothing to answer `kUnsupported`.
    using ExtensionHandler = std::function<std::optional<Reply>(const ExtensionRequest&)>;

    /// @brief Binds the local port.
    /// @param port The port on `127.0.0.1`; 0 picks a free one (see localPort()).
    /// @param token The secret of the requests.
    /// @param started When the server opened, for the uptime.
    /// @throws exceptions::SocketException if the port cannot be bound.
    AdminServer(std::uint16_t port, std::string token, TimePoint started);

    /// @brief Sets the handler of the simulation and world ranges.
    void setExtensionHandler(ExtensionHandler handler);

    /// @brief Answers the requests that arrived. Never blocks.
    /// @param now The current time.
    /// @param server The server to read and to command.
    /// @param game What the game side knows.
    void poll(TimePoint now, server::NetServer& server, const GameStatus& game);

    /// @brief Gives the port the socket is bound to.
    [[nodiscard]] std::uint16_t localPort() const;

  private:
    /// @brief Builds the reply to a decoded request.
    [[nodiscard]] Reply answer(const RequestFrame& frame, TimePoint now, server::NetServer& server,
                               const GameStatus& game);

    io::UdpSocket _socket;        ///< Bound to the loopback.
    std::string _token;           ///< The secret.
    TimePoint _started;           ///< For the uptime.
    ExtensionHandler _extension;  ///< Simulation and world requests; may be empty.
};
}  // namespace rtype::network::admin
