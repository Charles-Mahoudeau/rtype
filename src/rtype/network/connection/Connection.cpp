/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Connection
*/

#include "rtype/network/connection/Connection.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ratio>
#include <span>
#include <utility>
#include <vector>

#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/ByteReader.hpp"
#include "rtype/network/connection/ReliableSender.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/Packets.hpp"
#include "rtype/network/protocol/Sequence.hpp"

namespace rtype::network::connection {
namespace {
/// @brief Bytes taken by the kind byte and the fields of `Data` before its messages.
constexpr std::size_t kDataHeaderSize = 13;

/// @brief Bytes taken by the channel and the length in front of an unreliable message.
constexpr std::size_t kUnreliableHeaderSize = 3;

/// @brief Bytes taken by the channel, the id and the length in front of a reliable message.
constexpr std::size_t kReliableHeaderSize = 5;

/// @brief Weight of the RTT in the delay before a reliable message is sent again.
constexpr float kResendRttFactor = 1.25F;

/// @brief Appends a little-endian u16.
void writeU16(Bytes& body, const std::size_t value) {
    body.push_back(static_cast<std::byte>(value & 0xFFU));
    body.push_back(static_cast<std::byte>(value >> 8U));
}

/// @brief Room left for messages in a datagram.
constexpr std::size_t kMessagesCapacity = protocol::kMaxPacketSize - kDataHeaderSize;

/// @brief Number of sequences before `ack` that `ackBits` describes.
constexpr std::uint16_t kAckBitCount = 32;

/// @brief Weight of a new RTT sample in the smoothed value.
constexpr float kRttSmoothing = 0.1F;

/// @brief The ack sent while nothing was received: no datagram has this sequence yet, so it confirms nothing.
constexpr std::uint16_t kNoAck = 0xFFFF;
}  // namespace

Connection::Connection(const std::uint32_t sessionToken, const NetworkSettings& settings, const TimePoint now)
    : _sessionToken{sessionToken},
      _keepalive{settings.keepalive},
      _minResendInterval{settings.minResendInterval},
      _rtt{std::chrono::duration<float, std::milli>{settings.initialRtt}.count()},
      _lastSendTime{now},
      _lastReceiveTime{now} {}

bool Connection::send(const Channel channel, const std::span<const std::byte> payload) {
    if (!isValidWireValue(channel) || payload.empty() || payload.size() > protocol::kMaxMessageSize) {
        return false;
    }
    if (channel == Channel::kReliable) {
        if (_reliableSender.isOverflowed()) {
            return false;
        }
        _reliableSender.push(Bytes{payload.begin(), payload.end()});
        return true;
    }
    _queue.emplace_back(channel, Bytes{payload.begin(), payload.end()});
    return true;
}

bool Connection::receive(const protocol::Data& data, const TimePoint now) {
    std::vector<ReceivedMessage> messages;
    std::vector<std::pair<std::uint16_t, Bytes>> reliable;
    bytes::ByteReader reader{data.messages};
    while (reader.remainingSize() > 0) {
        const auto channel = static_cast<Channel>(reader.readU8());
        const std::uint16_t id = channel == Channel::kReliable ? reader.readU16() : 0;
        const std::size_t length = reader.readU16();
        if (!isValidWireValue(channel) || length == 0 || length > protocol::kMaxMessageSize) {
            return false;
        }
        const std::span<const std::byte> payload = reader.readBytes(length);
        if (!reader.isValid()) {
            return false;
        }
        if (channel == Channel::kReliable) {
            reliable.emplace_back(id, Bytes{payload.begin(), payload.end()});
        } else {
            messages.push_back(ReceivedMessage{.channel = channel, .payload = Bytes{payload.begin(), payload.end()}});
        }
    }

    registerReceived(data.sequence);
    processAcks(data.ack, data.ackBits, now);
    _lastReceiveTime = now;
    std::ranges::move(messages, std::back_inserter(_received));
    for (const auto& [id, payload] : reliable) {
        if (!_reliableReceiver.receive(id, payload)) {
            ++_protocolErrors;
        }
    }
    for (Bytes& payload : _reliableReceiver.takeDelivered()) {
        _received.push_back(ReceivedMessage{.channel = Channel::kReliable, .payload = std::move(payload)});
    }
    return true;
}

std::vector<Bytes> Connection::buildPackets(const TimePoint now) {
    const auto resendDelay =
        std::max(_minResendInterval, std::chrono::duration_cast<std::chrono::milliseconds>(
                                         std::chrono::duration<float, std::milli>{_rtt * kResendRttFactor}));
    std::vector<const PendingMessage*> due = _reliableSender.collectDue(now, resendDelay);
    std::ranges::stable_partition(due, [](const PendingMessage* message) { return message->sent; });

    std::vector<Bytes> datagrams;
    auto nextReliable = due.begin();
    auto nextUnreliable = _queue.begin();
    while ((nextReliable != due.end() || nextUnreliable != _queue.end()) &&
           datagrams.size() < protocol::kMaxDatagramsPerFlush) {
        Bytes body;
        body.reserve(kMessagesCapacity);
        std::vector<std::uint16_t> reliableIds;
        while (nextReliable != due.end() &&
               body.size() + kReliableHeaderSize + (*nextReliable)->payload.size() <= kMessagesCapacity) {
            const PendingMessage& message = **nextReliable;
            body.push_back(static_cast<std::byte>(Channel::kReliable));
            writeU16(body, message.id);
            writeU16(body, message.payload.size());
            body.insert(body.end(), message.payload.begin(), message.payload.end());
            reliableIds.push_back(message.id);
            ++nextReliable;
        }
        while (nextReliable == due.end() && nextUnreliable != _queue.end() &&
               body.size() + kUnreliableHeaderSize + nextUnreliable->second.size() <= kMessagesCapacity) {
            const auto& [channel, payload] = *nextUnreliable;
            body.push_back(static_cast<std::byte>(channel));
            writeU16(body, payload.size());
            body.insert(body.end(), payload.begin(), payload.end());
            ++nextUnreliable;
        }
        datagrams.push_back(makeDatagram(body, std::move(reliableIds), now));
    }
    _droppedMessages += static_cast<std::uint64_t>(std::distance(nextUnreliable, _queue.end()));
    _queue.clear();

    if (datagrams.empty() && now - _lastSendTime >= _keepalive) {
        datagrams.push_back(makeDatagram({}, {}, now));
    }
    return datagrams;
}

std::vector<ReceivedMessage> Connection::takeMessages() { return std::exchange(_received, {}); }

TimePoint Connection::lastReceiveTime() const noexcept { return _lastReceiveTime; }

std::chrono::duration<float, std::milli> Connection::rtt() const noexcept {
    return std::chrono::duration<float, std::milli>{_rtt};
}

std::uint64_t Connection::droppedMessageCount() const noexcept { return _droppedMessages; }

std::uint64_t Connection::protocolErrorCount() const noexcept { return _protocolErrors; }

bool Connection::isOverflowed() const noexcept { return _reliableSender.isOverflowed(); }

void Connection::registerReceived(const std::uint16_t sequence) noexcept {
    if (!_receivedAny) {
        _receivedAny = true;
        _ack = sequence;
        _ackBits = 0;
        return;
    }
    if (protocol::isNewer(sequence, _ack)) {
        if (const auto distance = static_cast<std::uint16_t>(sequence - _ack); distance > kAckBitCount) {
            _ackBits = 0;
        } else {
            _ackBits = distance == kAckBitCount ? 0U : _ackBits << distance;
            _ackBits |= 1U << (distance - 1U);
        }
        _ack = sequence;
        return;
    }
    const auto distance = static_cast<std::uint16_t>(_ack - sequence);
    if (distance >= 1 && distance <= kAckBitCount) {
        _ackBits |= 1U << (distance - 1U);
    }
}

void Connection::processAcks(const std::uint16_t ack, const std::uint32_t ackBits, const TimePoint now) {
    acknowledge(ack, now);
    for (std::uint16_t bit = 0; bit < kAckBitCount; ++bit) {
        if (((ackBits >> bit) & 1U) != 0) {
            acknowledge(static_cast<std::uint16_t>(ack - 1U - bit), now);
        }
    }
}

void Connection::acknowledge(const std::uint16_t sequence, const TimePoint now) {
    SentPacket* packet = _sent.find(sequence);
    if (packet == nullptr || packet->acked) {
        return;
    }
    packet->acked = true;
    for (const std::uint16_t id : packet->reliableIds) {
        _reliableSender.acknowledge(id);
    }
    const float sample = std::max(0.0F, std::chrono::duration<float, std::milli>{now - packet->sendTime}.count());
    _rtt += (sample - _rtt) * kRttSmoothing;
}

Bytes Connection::makeDatagram(const std::span<const std::byte> body, std::vector<std::uint16_t> reliableIds,
                               const TimePoint now) {
    for (const std::uint16_t id : reliableIds) {
        _reliableSender.markSent(id, now);
    }
    const protocol::Data data{
        .sessionToken = _sessionToken,
        .sequence = _nextSequence,
        .ack = _receivedAny ? _ack : kNoAck,
        .ackBits = _receivedAny ? _ackBits : 0U,
        .messages = body,
    };
    SentPacket& record = _sent.insert(_nextSequence);
    record.sendTime = now;
    record.reliableIds = std::move(reliableIds);
    ++_nextSequence;
    _lastSendTime = now;
    return protocol::PacketCodec::encode(data);
}
}  // namespace rtype::network::connection
