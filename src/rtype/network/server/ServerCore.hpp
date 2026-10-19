/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerCore
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <unordered_map>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/RandomSource.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/protocol/Packets.hpp"
#include "rtype/network/server/RoomMessages.hpp"
#include "rtype/network/server/Session.hpp"

namespace rtype::network::server {
/// @brief All the connection logic of a server, without a socket or a clock: handshake, sessions, routing, timeouts.
///
/// @details Datagrams, time and random numbers come in as parameters; the datagrams to send come out of
/// `takeDatagrams`. Nothing here throws, asserts or crashes on the bytes of a datagram: a bad one is dropped. With the
/// same `RandomSource`, datagrams and times, the output is identical.
class RTYPE_NETWORK_API ServerCore {
  public:
    /// @brief Builds a server core with no session.
    /// @param settings The limits and delays.
    /// @param random Where challenges and tokens come from.
    ServerCore(const NetworkSettings& settings, RandomSource random);

    /// @brief Destroys the core and its sessions, without telling the clients.
    ~ServerCore() = default;

    /// @brief A core owns the state of every session: it is not copied.
    ServerCore(const ServerCore&) = delete;

    /// @brief A core owns the state of every session: it is not copied.
    ServerCore& operator=(const ServerCore&) = delete;

    /// @brief Moves the whole state.
    ServerCore(ServerCore&&) noexcept = default;

    /// @brief Move-assigns the whole state.
    ServerCore& operator=(ServerCore&&) noexcept = default;

    /// @brief Sets the messages queued, reliably and in order, to every client accepted from now on.
    /// @param messages The messages, such as the manifest of replication.
    void setWelcomeMessages(std::vector<Bytes> messages);

    /// @brief Processes one datagram received from `from`; anything invalid is dropped silently.
    /// @param from The sender.
    /// @param datagram The bytes received; they need not outlive the call.
    /// @param now The time of arrival.
    void receive(const io::Address& from, std::span<const std::byte> datagram, TimePoint now);

    /// @brief Applies a command of the game; a command for a client that is gone is ignored.
    /// @param command The command.
    void execute(ServerCommand command);

    /// @brief Removes the expired challenges, then closes the silent and the overflowed sessions.
    /// @param now The current time.
    void update(TimePoint now);

    /// @brief Tells every client, three times, that the server stops, closes every session and ignores every
    /// datagram from now on.
    /// @param now The current time.
    void shutdown(TimePoint now);

    /// @brief Gives what must be written to the socket: the handshake replies and disconnects queued since the last
    /// call, then the datagrams of every session.
    /// @param now The current time.
    /// @return The datagrams, each with its destination.
    [[nodiscard]] std::vector<OutgoingDatagram> takeDatagrams(TimePoint now);

    /// @brief Gives the events for the game since the last call, in order, and forgets them.
    /// @return The events.
    [[nodiscard]] std::vector<ServerEvent> takeEvents();

    /// @brief Gives the number of sessions, welcoming or ready.
    [[nodiscard]] std::size_t sessionCount() const noexcept;

    /// @brief Gives the number of challenges awaiting their echo.
    [[nodiscard]] std::size_t pendingChallengeCount() const noexcept;

    /// @brief Gives how many `kData` datagrams were dropped as malformed.
    [[nodiscard]] std::uint64_t malformedPacketCount() const noexcept;

    /// @brief Gives how many messages were dropped because their type is not allowed in the state of the session.
    [[nodiscard]] std::uint64_t unexpectedMessageCount() const noexcept;

  private:
    /// @brief A challenge sent to an address and not echoed yet.
    struct PendingChallenge {
        std::uint64_t value;        ///< The random value to echo.
        TimePoint created;          ///< When it was sent.
        std::uint64_t order;        ///< Rank of creation, to find the oldest.
        ClientId resumeClient;      ///< The client the request wanted to resume (M4).
        std::uint64_t resumeToken;  ///< The proof of the right to resume (M4).
    };

    /// @brief Answers a `kConnectRequest`.
    void handleConnectRequest(const io::Address& from, const protocol::ConnectRequest& request, TimePoint now);

    /// @brief Answers a `kChallengeResponse`.
    void handleChallengeResponse(const io::Address& from, const protocol::ChallengeResponse& response, TimePoint now);

    /// @brief Gives a `kData` datagram to the connection of its session, then handles the messages it delivered.
    void handleData(const io::Address& from, const protocol::Data& data, TimePoint now);

    /// @brief Closes the session of a `kDisconnect` that carries the right token.
    void handleDisconnect(const io::Address& from, const protocol::Disconnect& disconnect);

    /// @brief Handles the messages delivered by the connection of `session`, according to its state.
    void processMessages(Session& session);

    /// @brief Creates the session of an accepted challenge and queues `Accepted` and the welcome messages.
    void accept(const io::Address& from, std::uint64_t challenge, TimePoint now);

    /// @brief Finds the session of `from` whose token is `token`.
    [[nodiscard]] Session* findSession(const io::Address& from, std::uint32_t token);

    /// @brief Gives a random non-zero session token.
    [[nodiscard]] std::uint32_t makeSessionToken() const;

    /// @brief Tells whether no more session may be created, except by replacing one of `replacing`.
    [[nodiscard]] bool isFull(bool replacing) const noexcept;

    /// @brief Removes the session `client` from every table and emits `ClientLeft` if the game knew it.
    void close(ClientId client, DisconnectReason reason);

    /// @brief Sends `Disconnect{reason}` three times to the session, then closes it.
    void disconnect(ClientId client, DisconnectReason reason);

    /// @brief Encodes `packet` and queues it for `to`.
    void reply(const io::Address& to, const protocol::Packet& packet);

    NetworkSettings _settings;                                    ///< Limits and delays.
    RandomSource _random;                                         ///< Source of challenges and tokens.
    std::vector<Bytes> _welcomeMessages;                          ///< Sent to every new session.
    std::unordered_map<io::Address, PendingChallenge> _pending;   ///< Challenges awaiting their echo.
    std::unordered_map<io::Address, ClientId> _clientsByAddress;  ///< Session of each address.
    std::map<ClientId, Session> _sessions;                        ///< Ordered, so the output is reproducible.
    ClientId _nextClient{1};                                      ///< Id of the next session.
    std::uint64_t _nextOrder{0};                                  ///< Rank of the next challenge.
    bool _isShutDown{false};                                      ///< True once `shutdown` ran.
    std::vector<OutgoingDatagram> _outgoing;                      ///< Replies and disconnects to send.
    std::vector<ServerEvent> _events;                             ///< Events for the game.
    std::uint64_t _malformedPackets{0};                           ///< Malformed `kData` dropped.
    std::uint64_t _unexpectedMessages{0};                         ///< Messages of a type not allowed.
};
}  // namespace rtype::network::server
