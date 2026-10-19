/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientCore
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/client/ClientEvent.hpp"
#include "rtype/network/connection/Connection.hpp"
#include "rtype/network/protocol/Packets.hpp"

namespace rtype::network::client {
/// @brief Where a client is between `connect` and the end of the connection.
enum class ClientState : std::uint8_t {
    kIdle,        ///< Never connected.
    kRequesting,  ///< `ConnectRequest` sent, a `Challenge` is awaited.
    kResponding,  ///< `ChallengeResponse` sent, `Accepted` is awaited.
    kConnected,   ///< Accepted: messages flow.
    kClosed,      ///< Ended, by the application, the server, a failure or a timeout.
};

/// @brief All the connection logic of a client, without a socket or a clock: handshake with retries, the connection
/// once accepted, detection of a silent server.
///
/// @details Datagrams and time come in as parameters; the datagrams to send come out of `takeDatagrams`. Nothing here
/// throws, asserts or crashes on the bytes of a datagram: a bad one, or one that does not fit the state, is dropped.
class RTYPE_NETWORK_API ClientCore {
  public:
    /// @brief Builds an idle client core.
    /// @param settings The delays: handshake retry and timeout, connection timeout and keepalive.
    explicit ClientCore(const NetworkSettings& settings);

    /// @brief Destroys the core and its connection, without telling the server.
    ~ClientCore() = default;

    /// @brief A core owns the state of its connection: it is not copied.
    ClientCore(const ClientCore&) = delete;

    /// @brief A core owns the state of its connection: it is not copied.
    ClientCore& operator=(const ClientCore&) = delete;

    /// @brief Moves the whole state.
    ClientCore(ClientCore&&) noexcept = default;

    /// @brief Move-assigns the whole state.
    ClientCore& operator=(ClientCore&&) noexcept = default;

    /// @brief Starts a connection from scratch and sends the first `ConnectRequest` at once.
    /// @param now The current time, start of the handshake timeout.
    /// @details Does nothing unless the state is `kIdle` or `kClosed`.
    void connect(TimePoint now);

    /// @brief Processes one datagram received from the server.
    /// @param datagram The bytes; copied where needed, so they need not outlive the call.
    /// @param now The time of arrival.
    void receive(std::span<const std::byte> datagram, TimePoint now);

    /// @brief Queues a message for the server.
    /// @param channel The channel to send it on.
    /// @param payload The bytes of the message, type byte included.
    /// @return False, and nothing queued, unless connected and the connection takes the message.
    bool send(Channel channel, std::span<const std::byte> payload);

    /// @brief Tells the server that this client loaded everything and can play, once. Does nothing unless connected.
    void sendReady();

    /// @brief Ends the connection on the application's request: sends `Disconnect{kGraceful}` three times, since
    /// nothing acknowledges it. No event is emitted. Does nothing unless connected.
    void disconnect(TimePoint now);

    /// @brief Checks the timeouts: the handshake gives up, a silent server is declared gone.
    /// @param now The current time.
    void update(TimePoint now);

    /// @brief Gives the datagrams to send now and forgets them: the handshake packets due, then the packets of the
    /// connection.
    /// @param now The current time.
    /// @return The datagrams, possibly none.
    [[nodiscard]] std::vector<Bytes> takeDatagrams(TimePoint now);

    /// @brief Gives the events since the last call, in order, and forgets them.
    /// @return The events.
    [[nodiscard]] std::vector<ClientEvent> takeEvents();

    /// @brief Gives the state of the connection.
    /// @return The state.
    [[nodiscard]] ClientState state() const noexcept;

    /// @brief Gives the id the server gave to this client.
    /// @return The id, or `kNoClient` before the first acceptance.
    [[nodiscard]] ClientId clientId() const noexcept;

    /// @brief Gives the token the server gave to resume the session later.
    /// @return The token, or 0 before the first acceptance.
    [[nodiscard]] std::uint64_t resumeToken() const noexcept;

    /// @brief Gives how many datagrams of the server held a malformed message.
    /// @return The number since creation.
    [[nodiscard]] std::uint64_t malformedPacketCount() const noexcept;

  private:
    /// @brief Queues the handshake packet of the current state and restarts the retry delay.
    void sendHandshakePacket(TimePoint now);

    /// @brief Queues `packet` for the server.
    void queue(const protocol::Packet& packet);

    void handleChallenge(const protocol::Challenge& challenge, TimePoint now);
    void handleAccepted(const protocol::Accepted& accepted, TimePoint now);
    void handleRejected(const protocol::Rejected& rejected);
    void handleData(const protocol::Data& data, TimePoint now);
    void handleDisconnect(const protocol::Disconnect& disconnect);

    /// @brief Turns the messages delivered by the connection into events.
    void processMessages();

    NetworkSettings _settings;                          ///< The delays.
    ClientState _state{ClientState::kIdle};             ///< Where the connection is.
    TimePoint _connectTime;                             ///< When `connect` was called.
    TimePoint _lastHandshakeSend;                       ///< When the handshake packet was last queued.
    std::uint64_t _challenge{0};                        ///< The latest challenge of the server.
    std::uint32_t _sessionToken{0};                     ///< Carried by every datagram once accepted.
    ClientId _client{kNoClient};                        ///< The id given by the server.
    std::uint64_t _resumeToken{0};                      ///< Given by the server to resume the session.
    bool _isReadySent{false};                           ///< True once `kReady` is queued.
    std::uint64_t _malformedPackets{0};                 ///< Datagrams of the server with a malformed message.
    std::optional<connection::Connection> _connection;  ///< Exists while connected.
    std::vector<Bytes> _outgoing;                       ///< Datagrams queued for `takeDatagrams`.
    std::vector<ClientEvent> _events;                   ///< Events queued for `takeEvents`.
};
}  // namespace rtype::network::client
