/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Connection
*/

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <ratio>
#include <span>
#include <utility>
#include <vector>

#include "rtype/network/Export.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/connection/ReliableReceiver.hpp"
#include "rtype/network/connection/ReliableSender.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/Packets.hpp"
#include "rtype/network/protocol/SequenceBuffer.hpp"

namespace rtype::network::connection {
/// @brief A message taken out of a received datagram.
struct ReceivedMessage {
    Channel channel;  ///< The channel the sender used.
    Bytes payload;    ///< The bytes of the message.
};

/// @brief Transport state for one peer: sequence numbers, acks, RTT, message packing. Never touches a socket or a
/// clock.
///
/// @details It does not know the address of the peer and does not check the session token of the datagrams it
/// receives: the core does that before calling it.
class RTYPE_NETWORK_API Connection {
  public:
    /// @brief Builds the state of a new connection.
    /// @param sessionToken The token written in every datagram built.
    /// @param settings The keepalive interval and the RTT assumed before the first measure.
    /// @param now The time of creation: the connection counts as just heard from and just sent to.
    Connection(std::uint32_t sessionToken, const NetworkSettings& settings, TimePoint now);

    /// @brief Destroys the connection and the messages it still holds.
    ~Connection() = default;

    /// @brief Copies the whole state.
    Connection(const Connection&) = default;

    /// @brief Copy-assigns the whole state.
    Connection& operator=(const Connection&) = default;

    /// @brief Moves the whole state.
    Connection(Connection&&) noexcept = default;

    /// @brief Move-assigns the whole state.
    Connection& operator=(Connection&&) noexcept = default;

    /// @brief Queues a copy of a message for the next `buildPackets`.
    /// @param channel The channel to send it on; a reliable message is resent until acked.
    /// @param payload The bytes of the message.
    /// @return False, and nothing queued, if the payload is empty or above `kMaxMessageSize`, or a reliable message
    /// arrives once the connection is overflowed.
    bool send(Channel channel, std::span<const std::byte> payload);

    /// @brief Processes one kData packet whose session token the caller already checked.
    /// @param data The packet; its messages are copied, so it need not outlive the call.
    /// @param now The time of arrival.
    /// @return False, and nothing changed, if one of its messages is malformed. A reliable message with an id far
    /// beyond the window is dropped and counted in `protocolErrorCount`, the rest of the datagram is kept.
    bool receive(const protocol::Data& data, TimePoint now);

    /// @brief Turns the queued messages into datagrams, or a keepalive when idle for too long.
    ///
    /// @details Reliable messages come first: those due again, then those never sent, then unreliable ones. A
    /// reliable message that does not fit stays pending. Unreliable messages that do not fit in `kMaxDatagramsPerFlush`
    /// datagrams are dropped and counted.
    /// @param now The current time.
    /// @return The datagrams to send now, possibly none.
    [[nodiscard]] std::vector<Bytes> buildPackets(TimePoint now);

    /// @brief Gives the messages delivered since the last call, in order of arrival, and forgets them.
    /// @return The delivered messages.
    [[nodiscard]] std::vector<ReceivedMessage> takeMessages();

    /// @brief Gives the time of the last datagram accepted by `receive`, or of creation before the first one.
    /// @return The time, used by the cores for timeouts.
    [[nodiscard]] TimePoint lastReceiveTime() const noexcept;

    /// @brief Gives the smoothed round-trip time.
    /// @return The RTT in milliseconds; `initialRtt` until the first measure.
    [[nodiscard]] std::chrono::duration<float, std::milli> rtt() const noexcept;

    /// @brief Gives how many unreliable messages were dropped because they did not fit in a flush.
    /// @return The number of dropped messages since creation.
    [[nodiscard]] std::uint64_t droppedMessageCount() const noexcept;

    /// @brief Gives how many reliable messages were dropped for an id beyond the window.
    /// @return The number of such messages since creation.
    [[nodiscard]] std::uint64_t protocolErrorCount() const noexcept;

    /// @brief Tells whether the peer lets too many reliable messages pile up unacknowledged.
    /// @return True, for good, once more than `kMaxPendingReliable` are pending; the core then disconnects.
    [[nodiscard]] bool isOverflowed() const noexcept;

  private:
    /// @brief What is remembered of one datagram sent.
    struct SentPacket {
        TimePoint sendTime;                      ///< When it was built.
        bool acked{false};                       ///< True once the peer confirmed it.
        std::vector<std::uint16_t> reliableIds;  ///< Ids of the reliable messages it carried.
    };

    /// @brief Updates `_ack` and `_ackBits` with the sequence of a datagram received.
    void registerReceived(std::uint16_t sequence) noexcept;

    /// @brief Marks the datagrams confirmed by the peer and measures the RTT on the first confirmation of each.
    void processAcks(std::uint16_t ack, std::uint32_t ackBits, TimePoint now);

    /// @brief Marks one datagram as acked and takes an RTT sample if it is the first time.
    void acknowledge(std::uint16_t sequence, TimePoint now);

    /// @brief Builds one datagram holding `body` as messages, with the next sequence number.
    [[nodiscard]] Bytes makeDatagram(std::span<const std::byte> body, std::vector<std::uint16_t> reliableIds,
                                     TimePoint now);

    std::uint32_t _sessionToken;                                               ///< Written in every datagram.
    std::chrono::milliseconds _keepalive;                                      ///< Idle time before a keepalive.
    std::chrono::milliseconds _minResendInterval;                              ///< Floor of the resend delay.
    std::uint16_t _nextSequence{0};                                            ///< Sequence of the next datagram.
    std::uint16_t _ack{0};                                                     ///< Newest sequence received.
    std::uint32_t _ackBits{0};                                                 ///< Bit N: `_ack - 1 - N` too.
    bool _receivedAny{false};                                                  ///< False until a datagram came.
    protocol::SequenceBuffer<SentPacket, protocol::kSentPacketHistory> _sent;  ///< Datagrams awaiting an ack.
    std::vector<std::pair<Channel, Bytes>> _queue;                             ///< Messages to send.
    std::vector<ReceivedMessage> _received;                                    ///< Messages to take.
    float _rtt;                                                                ///< Smoothed RTT, in ms.
    ReliableSender _reliableSender;                                            ///< Reliable messages to send.
    ReliableReceiver _reliableReceiver;                                        ///< Reliable messages received.
    std::uint64_t _droppedMessages{0};                                         ///< Unreliable messages dropped.
    std::uint64_t _protocolErrors{0};                                          ///< Reliable ids beyond the window.
    TimePoint _lastSendTime;                                                   ///< Last datagram built.
    TimePoint _lastReceiveTime;                                                ///< Last datagram accepted.
};
}  // namespace rtype::network::connection
