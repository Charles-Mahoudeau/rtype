/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetClient
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/client/ClientCore.hpp"
#include "rtype/network/client/ClientEvent.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/io/UdpSocket.hpp"

namespace rtype::network::client {
/// @brief A UDP socket glued to a `ClientCore`: what an application uses to talk to a server, driven by one `poll`
/// per tick. Datagrams from any address but the server's are dropped. Not thread-safe.
class RTYPE_NETWORK_API NetClient {
  public:
    /// @brief Binds a socket on any free port and remembers the server.
    /// @param server The address of the server.
    /// @param settings The delays.
    /// @throws exceptions::SocketException if the socket cannot be opened.
    NetClient(const io::Address& server, const NetworkSettings& settings);

    /// @brief Closes the socket, without telling the server (see `disconnect`).
    ~NetClient() = default;

    /// @brief A client owns a socket: it is not copied.
    NetClient(const NetClient&) = delete;

    /// @brief A client owns a socket: it is not copied.
    NetClient& operator=(const NetClient&) = delete;

    /// @brief A client owns a socket: it is not moved.
    NetClient(NetClient&&) = delete;

    /// @brief A client owns a socket: it is not moved.
    NetClient& operator=(NetClient&&) = delete;

    /// @brief Starts the handshake; the first request leaves at once.
    /// @param now The current time.
    void connect(TimePoint now);

    /// @brief Receives what arrived from the server, updates the core and sends what it has to send. Never blocks.
    /// @param now The current time.
    void poll(TimePoint now);

    /// @brief Queues a message for the server, sent at the next `poll` or `flush`.
    /// @param channel The channel to send it on.
    /// @param payload The bytes of the message, type byte included.
    /// @return False, and nothing queued, unless connected and the connection takes the message.
    bool send(Channel channel, std::span<const std::byte> payload);

    /// @brief Tells the server that this client loaded everything and can play.
    void sendReady();

    /// @brief Sends now what is queued, instead of waiting for the next `poll`.
    /// @param now The current time.
    void flush(TimePoint now);

    /// @brief Ends the connection: the three `Disconnect` leave at once.
    /// @param now The current time.
    void disconnect(TimePoint now);

    /// @brief Gives the events since the last call and forgets them.
    /// @return The events.
    [[nodiscard]] std::vector<ClientEvent> takeEvents();

    /// @brief Gives the port of the local socket.
    [[nodiscard]] std::uint16_t localPort() const;

    /// @brief Gives the core, to read the state of the connection and its counters.
    [[nodiscard]] const ClientCore& core() const noexcept;

  private:
    io::Address _server;    ///< The only address datagrams are accepted from.
    io::UdpSocket _socket;  ///< Where datagrams come from and go to.
    ClientCore _core;       ///< The logic.
};
}  // namespace rtype::network::client
