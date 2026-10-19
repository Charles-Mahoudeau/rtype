/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientCore
*/

#include "rtype/network/client/ClientCore.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/Overloaded.hpp"
#include "rtype/network/client/ClientEvent.hpp"
#include "rtype/network/connection/Connection.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/Packets.hpp"

namespace rtype::network::client {
namespace {
/// @brief The message type of `kReady`, the first payload byte.
constexpr std::uint8_t kReadyMessageType = 0x09;

/// @brief Times a `Disconnect` is sent, since nothing acknowledges it.
constexpr std::uint8_t kDisconnectRepeat = 3;
}  // namespace

ClientCore::ClientCore(const NetworkSettings& settings) : _settings{settings} {}

void ClientCore::connect(const TimePoint now) {
    if (_state != ClientState::kIdle && _state != ClientState::kClosed) {
        return;
    }
    _connection.reset();
    _challenge = 0;
    _sessionToken = 0;
    _client = kNoClient;
    _resumeToken = 0;
    _isReadySent = false;
    _connectTime = now;
    _state = ClientState::kRequesting;
    sendHandshakePacket(now);
}

void ClientCore::receive(const std::span<const std::byte> datagram, const TimePoint now) {
    const auto packet = protocol::PacketCodec::decode(datagram);
    if (!packet) {
        return;
    }
    std::visit(bytes::Overloaded{
                   [&](const protocol::Challenge& challenge) { handleChallenge(challenge, now); },
                   [&](const protocol::Accepted& accepted) { handleAccepted(accepted, now); },
                   [&](const protocol::Rejected& rejected) { handleRejected(rejected); },
                   [&](const protocol::Data& data) { handleData(data, now); },
                   [&](const protocol::Disconnect& disconnect) { handleDisconnect(disconnect); },
                   [](const auto&) {},
               },
               *packet);
}

bool ClientCore::send(const Channel channel, const std::span<const std::byte> payload) {
    return _state == ClientState::kConnected && _connection && _connection->send(channel, payload);
}

void ClientCore::sendReady() {
    if (_state != ClientState::kConnected || !_connection || _isReadySent) {
        return;
    }
    constexpr std::array message{static_cast<std::byte>(kReadyMessageType)};
    _isReadySent = _connection->send(Channel::kReliable, message);
}

void ClientCore::disconnect(const TimePoint /*now*/) {
    if (_state != ClientState::kConnected) {
        return;
    }
    for (std::uint8_t sent = 0; sent < kDisconnectRepeat; ++sent) {
        queue(protocol::Disconnect{.sessionToken = _sessionToken, .reason = DisconnectReason::kGraceful});
    }
    _connection.reset();
    _state = ClientState::kClosed;
}

void ClientCore::update(const TimePoint now) {
    if (_state == ClientState::kRequesting || _state == ClientState::kResponding) {
        if (now - _connectTime > _settings.handshakeTimeout) {
            _events.emplace_back(ConnectFailed{.reason = std::nullopt});
            _state = ClientState::kClosed;
        }
    } else if (_state == ClientState::kConnected && _connection &&
               now - _connection->lastReceiveTime() > _settings.timeout) {
        _events.emplace_back(Disconnected{.reason = DisconnectReason::kTimeout});
        _connection.reset();
        _state = ClientState::kClosed;
    }
}

std::vector<Bytes> ClientCore::takeDatagrams(const TimePoint now) {
    if ((_state == ClientState::kRequesting || _state == ClientState::kResponding) &&
        now - _lastHandshakeSend >= _settings.handshakeRetry) {
        sendHandshakePacket(now);
    }
    std::vector<Bytes> result = std::move(_outgoing);
    _outgoing.clear();
    if (_state == ClientState::kConnected && _connection) {
        for (Bytes& bytes : _connection->buildPackets(now)) {
            result.push_back(std::move(bytes));
        }
    }
    return result;
}

std::vector<ClientEvent> ClientCore::takeEvents() {
    std::vector<ClientEvent> result = std::move(_events);
    _events.clear();
    return result;
}

ClientState ClientCore::state() const noexcept { return _state; }

ClientId ClientCore::clientId() const noexcept { return _client; }

std::uint64_t ClientCore::resumeToken() const noexcept { return _resumeToken; }

std::uint64_t ClientCore::malformedPacketCount() const noexcept { return _malformedPackets; }

void ClientCore::sendHandshakePacket(const TimePoint now) {
    if (_state == ClientState::kRequesting) {
        queue(protocol::ConnectRequest{.version = protocol::kProtocolVersion});
    } else {
        queue(protocol::ChallengeResponse{.challenge = _challenge});
    }
    _lastHandshakeSend = now;
}

void ClientCore::queue(const protocol::Packet& packet) {
    std::visit(
        [&](const auto& alternative) {
            if (Bytes bytes = protocol::PacketCodec::encode(alternative); !bytes.empty()) {
                _outgoing.push_back(std::move(bytes));
            }
        },
        packet);
}

void ClientCore::handleChallenge(const protocol::Challenge& challenge, const TimePoint now) {
    if (_state != ClientState::kRequesting && _state != ClientState::kResponding) {
        return;
    }
    _challenge = challenge.challenge;
    _state = ClientState::kResponding;
    sendHandshakePacket(now);
}

void ClientCore::handleAccepted(const protocol::Accepted& accepted, const TimePoint now) {
    if (_state != ClientState::kResponding) {
        return;
    }
    _sessionToken = accepted.sessionToken;
    _client = accepted.client;
    _resumeToken = accepted.resumeToken;
    _connection.emplace(_sessionToken, _settings, now);
    _state = ClientState::kConnected;
    _events.emplace_back(ConnectionAccepted{.client = _client});
}

void ClientCore::handleRejected(const protocol::Rejected& rejected) {
    if (_state != ClientState::kRequesting && _state != ClientState::kResponding) {
        return;
    }
    _events.emplace_back(ConnectFailed{.reason = rejected.reason});
    _state = ClientState::kClosed;
}

void ClientCore::handleData(const protocol::Data& data, const TimePoint now) {
    if (_state != ClientState::kConnected || !_connection || data.sessionToken != _sessionToken) {
        return;
    }
    if (!_connection->receive(data, now)) {
        ++_malformedPackets;
        return;
    }
    processMessages();
}

void ClientCore::handleDisconnect(const protocol::Disconnect& disconnect) {
    if (_state != ClientState::kConnected || disconnect.sessionToken != _sessionToken) {
        return;
    }
    _events.emplace_back(Disconnected{.reason = disconnect.reason});
    _connection.reset();
    _state = ClientState::kClosed;
}

void ClientCore::processMessages() {
    if (!_connection) {
        return;
    }
    for (auto& [channel, payload] : _connection->takeMessages()) {
        _events.emplace_back(ServerMessage{.channel = channel, .payload = std::move(payload)});
    }
}
}  // namespace rtype::network::client
