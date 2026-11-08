/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NetServer
*/

#pragma once

#include <cstdint>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/RandomSource.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/io/SocketStats.hpp"
#include "rtype/network/io/UdpSocket.hpp"
#include "rtype/network/server/ClientInfo.hpp"
#include "rtype/network/server/RoomMessages.hpp"
#include "rtype/network/server/ServerCore.hpp"
#include "rtype/network/simulation/LinkSimulator.hpp"

namespace rtype::network::server {
/// @brief A UDP socket glued to a `ServerCore`: what an application uses to serve clients, driven by one `poll` per
/// tick. Not thread-safe.
class RTYPE_NETWORK_API NetServer {
  public:
    /// @brief Binds the socket and creates the core.
    /// @param settings The port, limits and delays.
    /// @throws exceptions::SocketException if the port cannot be bound.
    explicit NetServer(const NetworkSettings& settings);

    /// @brief Binds the socket and creates the core, with the random numbers of the link simulation fixed (tests).
    /// @param settings The port, limits and delays.
    /// @param simulationRandom Where the drops and the jitter of setSimulation() come from.
    /// @throws exceptions::SocketException if the port cannot be bound.
    NetServer(const NetworkSettings& settings, RandomSource simulationRandom);

    /// @brief Closes the socket, without telling the clients (see `shutdown`).
    ~NetServer() = default;

    /// @brief A server owns a socket: it is not copied.
    NetServer(const NetServer&) = delete;

    /// @brief A server owns a socket: it is not copied.
    NetServer& operator=(const NetServer&) = delete;

    /// @brief A server owns a socket: it is not moved.
    NetServer(NetServer&&) = delete;

    /// @brief A server owns a socket: it is not moved.
    NetServer& operator=(NetServer&&) = delete;

    /// @brief Sets the messages queued, reliably and in order, to every client accepted from now on.
    /// @param messages The messages, such as the manifest of replication.
    void setWelcomeMessages(std::vector<Bytes> messages);

    /// @brief Receives what arrived, updates the core and sends what it has to send. Never blocks.
    /// @param now The current time.
    void poll(TimePoint now);

    /// @brief Applies a command of the game; its datagrams leave at the next `poll` or `flush`.
    /// @param command The command.
    /// @return For a `SendMessage`, whether at least one ready client queued it; for a `KickClient`, whether the
    /// client existed.
    bool execute(ServerCommand command);

    /// @brief Sends now what the commands queued, instead of waiting for the next `poll`.
    /// @param now The current time.
    void flush(TimePoint now);

    /// @brief Tells the clients that the server stops, and sends it at once.
    /// @param now The current time.
    void shutdown(TimePoint now);

    /// @brief Gives the events since the last call and forgets them.
    /// @return The events.
    [[nodiscard]] std::vector<ServerEvent> takeEvents();

    /// @brief Gives the port the socket is bound to.
    [[nodiscard]] std::uint16_t localPort() const;

    /// @brief Gives a copy of what is known of each connected client, by client id.
    [[nodiscard]] std::vector<ClientInfo> clients() const;

    /// @brief Gives the counters of the socket: datagrams and bytes, received and sent.
    [[nodiscard]] io::SocketStats socketStats() const noexcept;

    /// @brief Imitates a bad link on the game socket: latency, jitter and loss, on what is received and on what is
    /// sent. Off by default, and then free: the datagrams do not go through the simulation.
    /// @param settings The wanted settings, brought into their ranges.
    /// @return The settings in force.
    simulation::SimulationSettings setSimulation(const simulation::SimulationSettings& settings);

    /// @return The settings of the link simulation in force.
    [[nodiscard]] const simulation::SimulationSettings& simulation() const noexcept;

    /// @brief Gives the core, to read its counters.
    [[nodiscard]] const ServerCore& core() const noexcept;

  private:
    io::UdpSocket _socket;                ///< Where datagrams come from and go to.
    ServerCore _core;                     ///< The logic.
    simulation::LinkSimulator _inbound;   ///< Holds back what is received.
    simulation::LinkSimulator _outbound;  ///< Holds back what is sent.
};
}  // namespace rtype::network::server
