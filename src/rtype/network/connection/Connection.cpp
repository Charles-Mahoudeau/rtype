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
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/Packets.hpp"
#include "rtype/network/protocol/Sequence.hpp"

namespace rtype::network::connection {
namespace {
/// @brief Bytes taken by the kind byte and the fields of `Data` before its messages.
constexpr std::size_t kDataHeaderSize = 13;

/// @brief Bytes taken by the channel and the length in front of each message.
constexpr std::size_t kMessageHeaderSize = 3;

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
      _rtt{std::chrono::duration<float, std::milli>{settings.initialRtt}.count()},
      _lastSendTime{now},
      _lastReceiveTime{now} {}

bool Connection::send(const Channel channel, const std::span<const std::byte> payload) {
    if (channel != Channel::kUnreliable || payload.empty() || payload.size() > protocol::kMaxMessageSize) {
        return false;
    }
    _queue.emplace_back(channel, Bytes{payload.begin(), payload.end()});
    return true;
}

bool Connection::receive(const protocol::Data& data, const TimePoint now) {
    std::vector<ReceivedMessage> messages;
    bytes::ByteReader reader{data.messages};
    while (reader.remainingSize() > 0) {
        const auto channel = static_cast<Channel>(reader.readU8());
        const std::size_t length = reader.readU16();
        if (!isValidWireValue(channel) || length == 0 || length > protocol::kMaxMessageSize) {
            return false;
        }
        const std::span<const std::byte> payload = reader.readBytes(length);
        if (!reader.isValid()) {
            return false;
        }
        messages.push_back(ReceivedMessage{.channel = channel, .payload = Bytes{payload.begin(), payload.end()}});
    }

    registerReceived(data.sequence);
    processAcks(data.ack, data.ackBits, now);
    _lastReceiveTime = now;
    std::ranges::move(messages, std::back_inserter(_received));
    return true;
}

std::vector<Bytes> Connection::buildPackets(const TimePoint now) {
    std::vector<Bytes> datagrams;
    auto next = _queue.begin();
    while (next != _queue.end() && datagrams.size() < protocol::kMaxDatagramsPerFlush) {
        Bytes body;
        body.reserve(kMessagesCapacity);
        while (next != _queue.end() && body.size() + kMessageHeaderSize + next->second.size() <= kMessagesCapacity) {
            const auto& [channel, payload] = *next;
            body.push_back(static_cast<std::byte>(channel));
            body.push_back(static_cast<std::byte>(payload.size() & 0xFFU));
            body.push_back(static_cast<std::byte>(payload.size() >> 8U));
            body.insert(body.end(), payload.begin(), payload.end());
            ++next;
        }
        datagrams.push_back(makeDatagram(body, now));
    }
    _droppedMessages += static_cast<std::uint64_t>(std::distance(next, _queue.end()));
    _queue.clear();

    if (datagrams.empty() && now - _lastSendTime >= _keepalive) {
        datagrams.push_back(makeDatagram({}, now));
    }
    return datagrams;
}

std::vector<ReceivedMessage> Connection::takeMessages() { return std::exchange(_received, {}); }

TimePoint Connection::lastReceiveTime() const noexcept { return _lastReceiveTime; }

std::chrono::duration<float, std::milli> Connection::rtt() const noexcept {
    return std::chrono::duration<float, std::milli>{_rtt};
}

std::uint64_t Connection::droppedMessageCount() const noexcept { return _droppedMessages; }

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
    const float sample = std::max(0.0F, std::chrono::duration<float, std::milli>{now - packet->sendTime}.count());
    _rtt += (sample - _rtt) * kRttSmoothing;
}

Bytes Connection::makeDatagram(const std::span<const std::byte> body, const TimePoint now) {
    const protocol::Data data{
        .sessionToken = _sessionToken,
        .sequence = _nextSequence,
        .ack = _receivedAny ? _ack : kNoAck,
        .ackBits = _receivedAny ? _ackBits : 0U,
        .messages = body,
    };
    SentPacket& record = _sent.insert(_nextSequence);
    record.sendTime = now;
    ++_nextSequence;
    _lastSendTime = now;
    return protocol::PacketCodec::encode(data);
}
}  // namespace rtype::network::connection
