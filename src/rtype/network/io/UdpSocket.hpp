/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UdpSocket
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/io/SocketStats.hpp"

namespace rtype::network::io {
/// @brief A datagram received, with its sender.
struct Datagram {
    Address from;  ///< Who sent it.
    Bytes bytes;   ///< Its content.
};

/// @brief The only code that talks to the network: an IPv4 UDP socket that keeps receiving whatever happens.
///
/// @details Receiving is asynchronous on an Asio `io_context`: the caller polls once per tick, or runs it on an I/O
/// thread. After any receive error but `operation_aborted` (on Windows, `connection_reset` after a datagram sent to
/// a crashed peer) the next receive is started, so one bad peer never stops the others. Not thread-safe: one thread
/// owns it.
class RTYPE_NETWORK_API UdpSocket {
  public:
    /// @brief Opens an IPv4 UDP socket bound to `0.0.0.0:port` and starts receiving.
    /// @param port The port to bind; 0 picks any free port (for clients).
    /// @throws exceptions::SocketException if the port cannot be bound.
    explicit UdpSocket(std::uint16_t port);

    /// @brief Closes the socket.
    ~UdpSocket();

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;
    UdpSocket(UdpSocket&&) = delete;
    UdpSocket& operator=(UdpSocket&&) = delete;

    /// @brief Sends a datagram, synchronously. A failure is counted, never thrown.
    /// @param to The destination.
    /// @param bytes The content.
    void sendTo(const Address& to, std::span<const std::byte> bytes) noexcept;

    /// @brief Runs the handlers that are ready and returns at once; never blocks.
    void poll();

    /// @brief Blocks and processes completions until `stop()`.
    void run();

    /// @brief Cancels the pending receive, which is not re-armed; `run()` returns.
    void stop() noexcept;

    /// @brief Gives the datagrams received since the last call and empties the list.
    [[nodiscard]] std::vector<Datagram> takeReceived();

    /// @brief Gives the port the socket is bound to (useful when 0 was asked).
    [[nodiscard]] std::uint16_t localPort() const;

    /// @brief Gives the counters.
    [[nodiscard]] SocketStats stats() const noexcept;

  private:
    struct Impl;  ///< io_context, socket, receive buffer: the only place Asio appears.

    std::unique_ptr<Impl> _impl;  ///< Hides Asio.
};
}  // namespace rtype::network::io
