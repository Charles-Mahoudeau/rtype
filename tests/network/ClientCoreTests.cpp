/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ClientCoreTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

#include "FakeClock.hpp"
#include "SimulatedLink.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/client/ClientCore.hpp"
#include "rtype/network/client/ClientEvent.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/Packets.hpp"
#include "rtype/network/server/RoomMessages.hpp"
#include "rtype/network/server/ServerCore.hpp"

namespace network = rtype::network;
namespace protocol = rtype::network::protocol;
namespace client = rtype::network::client;
namespace server = rtype::network::server;
namespace tests = rtype::network::tests;

namespace {
constexpr std::uint8_t kGameType = 0x20;
constexpr auto kStep = std::chrono::milliseconds{10};

network::io::Address makeClientAddress() {
    network::io::Address::Bytes16 bytes{};
    bytes[10] = 0xFF;
    bytes[11] = 0xFF;
    bytes[12] = 10;
    bytes[15] = 1;
    return network::io::Address{bytes, 5000};
}

network::Bytes message(const std::uint8_t type, const std::uint8_t fill = 7) {
    return network::Bytes{static_cast<std::byte>(type), static_cast<std::byte>(fill)};
}

/// @brief The packets of `datagrams` that are a `P`, decoded.
template <typename P>
std::vector<P> packetsOf(const std::vector<network::Bytes>& datagrams) {
    std::vector<P> result;
    for (const auto& datagram : datagrams) {
        const auto packet = protocol::PacketCodec::decode(datagram);
        if (packet && std::holds_alternative<P>(*packet)) {
            result.push_back(std::get<P>(*packet));
        }
    }
    return result;
}

/// @brief The events of `events` that are an `E`.
template <typename E, typename Variant>
std::vector<E> eventsOf(const std::vector<Variant>& events) {
    std::vector<E> result;
    for (const auto& event : events) {
        if (const auto* found = std::get_if<E>(&event)) {
            result.push_back(*found);
        }
    }
    return result;
}

/// @brief A `ClientCore` and a `ServerCore` joined by a `SimulatedLink`: endpoint 0 is the client, 1 the server.
class ClientCoreTest : public ::testing::Test {
  protected:
    ClientCoreTest() { rebuild(); }

    void rebuild() {
        std::uint64_t counter = 1000;
        _server.emplace(_settings, [counter]() mutable { return counter++; });
        _client.emplace(_settings);
        _link.emplace(tests::LinkConditions{}, 42);
    }

    /// @brief Moves the time by one step and lets both sides receive, update and send.
    void step() {
        _clock.advance(kStep);
        const auto now = _clock.now();
        for (const auto& datagram : _link->receive(1, now)) {
            _server->receive(_address, datagram, now);
        }
        for (const auto& datagram : _link->receive(0, now)) {
            _client->receive(datagram, now);
        }
        _server->update(now);
        _client->update(now);
        for (auto& datagram : _server->takeDatagrams(now)) {
            _link->send(0, std::move(datagram.bytes), now);
        }
        for (auto& datagram : _client->takeDatagrams(now)) {
            _link->send(1, std::move(datagram), now);
        }
        for (auto& event : _client->takeEvents()) {
            _clientEvents.push_back(std::move(event));
        }
        for (auto& event : _server->takeEvents()) {
            _serverEvents.push_back(std::move(event));
        }
    }

    void run(const std::chrono::milliseconds duration) {
        for (auto elapsed = std::chrono::milliseconds{0}; elapsed < duration; elapsed += kStep) {
            step();
        }
    }

    /// @brief Steps until the client left the handshake, or `limit` is reached.
    void runHandshake(const std::chrono::milliseconds limit = std::chrono::seconds{6}) {
        for (auto elapsed = std::chrono::milliseconds{0};
             elapsed < limit && _client->state() != client::ClientState::kConnected &&
             _client->state() != client::ClientState::kClosed;
             elapsed += kStep) {
            step();
        }
    }

    void connect() {
        _client->connect(_clock.now());
        runHandshake();
        ASSERT_EQ(_client->state(), client::ClientState::kConnected);
    }

    void receiveFromServer(const network::Bytes& datagram) { _client->receive(datagram, _clock.now()); }

    network::NetworkSettings _settings;
    tests::FakeClock _clock;
    network::io::Address _address{makeClientAddress()};
    std::optional<server::ServerCore> _server;
    std::optional<client::ClientCore> _client;
    std::optional<tests::SimulatedLink> _link;
    std::vector<client::ClientEvent> _clientEvents;
    std::vector<server::ServerEvent> _serverEvents;
};
}  // namespace

TEST_F(ClientCoreTest, StartsIdleWithoutClientId) {
    EXPECT_EQ(_client->state(), client::ClientState::kIdle);
    EXPECT_EQ(_client->clientId(), network::kNoClient);
    EXPECT_TRUE(_client->takeDatagrams(_clock.now()).empty());
    EXPECT_TRUE(_client->takeEvents().empty());
}

TEST_F(ClientCoreTest, ConnectSendsRequestAtOnce) {
    _client->connect(_clock.now());
    EXPECT_EQ(_client->state(), client::ClientState::kRequesting);
    const auto requests = packetsOf<protocol::ConnectRequest>(_client->takeDatagrams(_clock.now()));
    ASSERT_EQ(requests.size(), 1U);
    EXPECT_EQ(requests.front().version, protocol::kProtocolVersion);
    EXPECT_EQ(requests.front().resumeClient, network::kNoClient);
    EXPECT_EQ(requests.front().resumeToken, 0U);
}

TEST_F(ClientCoreTest, RequestIsRepeatedEveryHandshakeRetry) {
    _client->connect(_clock.now());
    EXPECT_EQ(_client->takeDatagrams(_clock.now()).size(), 1U);
    _clock.advance(_settings.handshakeRetry / 2);
    EXPECT_TRUE(_client->takeDatagrams(_clock.now()).empty());
    _clock.advance(_settings.handshakeRetry / 2);
    EXPECT_EQ(packetsOf<protocol::ConnectRequest>(_client->takeDatagrams(_clock.now())).size(), 1U);
    _clock.advance(_settings.handshakeRetry);
    EXPECT_EQ(packetsOf<protocol::ConnectRequest>(_client->takeDatagrams(_clock.now())).size(), 1U);
}

TEST_F(ClientCoreTest, ConnectIsIgnoredWhileHandshaking) {
    _client->connect(_clock.now());
    EXPECT_EQ(_client->takeDatagrams(_clock.now()).size(), 1U);
    _client->connect(_clock.now());
    EXPECT_TRUE(_client->takeDatagrams(_clock.now()).empty());
}

TEST_F(ClientCoreTest, ChallengeIsAnsweredAtOnceThenRepeated) {
    _client->connect(_clock.now());
    (void)_client->takeDatagrams(_clock.now());
    receiveFromServer(protocol::PacketCodec::encode(protocol::Challenge{.challenge = 77}));
    EXPECT_EQ(_client->state(), client::ClientState::kResponding);
    auto responses = packetsOf<protocol::ChallengeResponse>(_client->takeDatagrams(_clock.now()));
    ASSERT_EQ(responses.size(), 1U);
    EXPECT_EQ(responses.front().challenge, 77U);

    _clock.advance(_settings.handshakeRetry);
    const auto datagrams = _client->takeDatagrams(_clock.now());
    EXPECT_TRUE(packetsOf<protocol::ConnectRequest>(datagrams).empty());
    responses = packetsOf<protocol::ChallengeResponse>(datagrams);
    ASSERT_EQ(responses.size(), 1U);
    EXPECT_EQ(responses.front().challenge, 77U);
}

TEST_F(ClientCoreTest, NewChallengeReplacesTheOldOne) {
    _client->connect(_clock.now());
    (void)_client->takeDatagrams(_clock.now());
    receiveFromServer(protocol::PacketCodec::encode(protocol::Challenge{.challenge = 5}));
    receiveFromServer(protocol::PacketCodec::encode(protocol::Challenge{.challenge = 9}));
    const auto responses = packetsOf<protocol::ChallengeResponse>(_client->takeDatagrams(_clock.now()));
    ASSERT_EQ(responses.size(), 2U);
    EXPECT_EQ(responses.front().challenge, 5U);
    EXPECT_EQ(responses.back().challenge, 9U);

    _clock.advance(_settings.handshakeRetry);
    const auto repeated = packetsOf<protocol::ChallengeResponse>(_client->takeDatagrams(_clock.now()));
    ASSERT_EQ(repeated.size(), 1U);
    EXPECT_EQ(repeated.front().challenge, 9U);
}

TEST_F(ClientCoreTest, HandshakeSucceedsThroughServerCore) {
    _client->connect(_clock.now());
    runHandshake();
    EXPECT_EQ(_client->state(), client::ClientState::kConnected);
    EXPECT_NE(_client->clientId(), network::kNoClient);
    EXPECT_NE(_client->resumeToken(), 0U);
    const auto accepted = eventsOf<client::ConnectionAccepted>(_clientEvents);
    ASSERT_EQ(accepted.size(), 1U);
    EXPECT_EQ(accepted.front().client, _client->clientId());
    EXPECT_EQ(_server->sessionCount(), 1U);
}

TEST_F(ClientCoreTest, HandshakeSucceedsUnderHeavyLoss) {
    _link.emplace(tests::LinkConditions{.loss = 0.5}, 7);
    _client->connect(_clock.now());
    runHandshake();
    EXPECT_EQ(_client->state(), client::ClientState::kConnected);
    EXPECT_EQ(eventsOf<client::ConnectionAccepted>(_clientEvents).size(), 1U);
    EXPECT_TRUE(eventsOf<client::ConnectFailed>(_clientEvents).empty());
}

TEST_F(ClientCoreTest, DuplicateAcceptedIsIgnored) {
    _link.emplace(tests::LinkConditions{.duplicate = 1.0}, 3);
    _client->connect(_clock.now());
    runHandshake();
    run(std::chrono::milliseconds{200});
    EXPECT_EQ(_client->state(), client::ClientState::kConnected);
    EXPECT_EQ(eventsOf<client::ConnectionAccepted>(_clientEvents).size(), 1U);
}

TEST_F(ClientCoreTest, VersionMismatchFailsTheConnection) {
    _client->connect(_clock.now());
    receiveFromServer(
        protocol::PacketCodec::encode(protocol::Rejected{.reason = network::RejectReason::kVersionMismatch}));
    EXPECT_EQ(_client->state(), client::ClientState::kClosed);
    const auto failed = eventsOf<client::ConnectFailed>(_client->takeEvents());
    ASSERT_EQ(failed.size(), 1U);
    EXPECT_EQ(failed.front().reason, network::RejectReason::kVersionMismatch);
}

TEST_F(ClientCoreTest, ServerFullFailsTheConnection) {
    _settings.maxClients = 0;
    rebuild();
    _client->connect(_clock.now());
    runHandshake();
    EXPECT_EQ(_client->state(), client::ClientState::kClosed);
    const auto failed = eventsOf<client::ConnectFailed>(_clientEvents);
    ASSERT_EQ(failed.size(), 1U);
    EXPECT_EQ(failed.front().reason, network::RejectReason::kServerFull);
}

TEST_F(ClientCoreTest, RejectedWhileRespondingFailsTheConnection) {
    _client->connect(_clock.now());
    receiveFromServer(protocol::PacketCodec::encode(protocol::Challenge{.challenge = 1}));
    receiveFromServer(protocol::PacketCodec::encode(protocol::Rejected{.reason = network::RejectReason::kServerFull}));
    EXPECT_EQ(_client->state(), client::ClientState::kClosed);
    EXPECT_EQ(eventsOf<client::ConnectFailed>(_client->takeEvents()).size(), 1U);
}

TEST_F(ClientCoreTest, SilentServerFailsAfterHandshakeTimeout) {
    _link.emplace(tests::LinkConditions{.loss = 1.0}, 1);
    _client->connect(_clock.now());
    run(_settings.handshakeTimeout - kStep);
    EXPECT_EQ(_client->state(), client::ClientState::kRequesting);
    EXPECT_TRUE(_clientEvents.empty());
    run(std::chrono::milliseconds{2 * kStep.count()});
    EXPECT_EQ(_client->state(), client::ClientState::kClosed);
    const auto failed = eventsOf<client::ConnectFailed>(_clientEvents);
    ASSERT_EQ(failed.size(), 1U);
    EXPECT_FALSE(failed.front().reason);
}

TEST_F(ClientCoreTest, ClosedClientCanConnectAgain) {
    _link.emplace(tests::LinkConditions{.loss = 1.0}, 1);
    _client->connect(_clock.now());
    run(_settings.handshakeTimeout + kStep * 2);
    ASSERT_EQ(_client->state(), client::ClientState::kClosed);
    _clientEvents.clear();

    _link.emplace(tests::LinkConditions{}, 1);
    _client->connect(_clock.now());
    runHandshake();
    EXPECT_EQ(_client->state(), client::ClientState::kConnected);
    EXPECT_EQ(eventsOf<client::ConnectionAccepted>(_clientEvents).size(), 1U);
}

TEST_F(ClientCoreTest, SendFailsBeforeAcceptance) {
    const network::Bytes payload = message(kGameType);
    EXPECT_FALSE(_client->send(network::Channel::kReliable, payload));
    _client->connect(_clock.now());
    EXPECT_FALSE(_client->send(network::Channel::kReliable, payload));
    receiveFromServer(protocol::PacketCodec::encode(protocol::Challenge{.challenge = 1}));
    EXPECT_FALSE(_client->send(network::Channel::kUnreliable, payload));
}

TEST_F(ClientCoreTest, MessagesFlowBothWaysAfterReady) {
    connect();
    _client->sendReady();
    run(std::chrono::milliseconds{100});
    const auto joined = eventsOf<server::ClientJoined>(_serverEvents);
    ASSERT_EQ(joined.size(), 1U);
    EXPECT_EQ(joined.front().client, _client->clientId());

    EXPECT_TRUE(_client->send(network::Channel::kReliable, message(kGameType, 1)));
    _server->execute(server::SendMessage{
        .client = _client->clientId(), .channel = network::Channel::kReliable, .payload = message(kGameType, 2)});
    run(std::chrono::milliseconds{100});

    const auto fromClient = eventsOf<server::ClientMessage>(_serverEvents);
    ASSERT_EQ(fromClient.size(), 1U);
    EXPECT_EQ(fromClient.front().payload, message(kGameType, 1));
    const auto fromServer = eventsOf<client::ServerMessage>(_clientEvents);
    ASSERT_EQ(fromServer.size(), 1U);
    EXPECT_EQ(fromServer.front().channel, network::Channel::kReliable);
    EXPECT_EQ(fromServer.front().payload, message(kGameType, 2));
}

TEST_F(ClientCoreTest, ReadyIsSentOnce) {
    connect();
    _client->sendReady();
    _client->sendReady();
    run(std::chrono::milliseconds{100});
    EXPECT_EQ(eventsOf<server::ClientJoined>(_serverEvents).size(), 1U);
    EXPECT_EQ(_server->unexpectedMessageCount(), 0U);
}

TEST_F(ClientCoreTest, ReadyBeforeAcceptanceDoesNothing) {
    _client->sendReady();
    _client->connect(_clock.now());
    _client->sendReady();
    runHandshake();
    run(std::chrono::milliseconds{100});
    EXPECT_TRUE(eventsOf<server::ClientJoined>(_serverEvents).empty());
}

TEST_F(ClientCoreTest, ServerShutdownReachesTheClient) {
    connect();
    _client->sendReady();
    run(std::chrono::milliseconds{100});
    _server->shutdown(_clock.now());
    run(std::chrono::milliseconds{100});
    EXPECT_EQ(_client->state(), client::ClientState::kClosed);
    const auto disconnected = eventsOf<client::Disconnected>(_clientEvents);
    ASSERT_EQ(disconnected.size(), 1U);
    EXPECT_EQ(disconnected.front().reason, network::DisconnectReason::kServerShutdown);
}

TEST_F(ClientCoreTest, ServerSilenceTimesOut) {
    connect();
    run(std::chrono::milliseconds{200});
    _link->setConditions(tests::LinkConditions{.loss = 1.0});
    run(_settings.timeout - std::chrono::milliseconds{300});
    EXPECT_EQ(_client->state(), client::ClientState::kConnected);
    run(std::chrono::milliseconds{600});
    EXPECT_EQ(_client->state(), client::ClientState::kClosed);
    const auto disconnected = eventsOf<client::Disconnected>(_clientEvents);
    ASSERT_EQ(disconnected.size(), 1U);
    EXPECT_EQ(disconnected.front().reason, network::DisconnectReason::kTimeout);
}

TEST_F(ClientCoreTest, KeepaliveKeepsTheConnectionAlive) {
    connect();
    run(_settings.timeout * 3);
    EXPECT_EQ(_client->state(), client::ClientState::kConnected);
    EXPECT_TRUE(eventsOf<client::Disconnected>(_clientEvents).empty());
}

TEST_F(ClientCoreTest, DisconnectSendsThreeGracefulPacketsAndNoEvent) {
    connect();
    _clientEvents.clear();
    _client->disconnect(_clock.now());
    EXPECT_EQ(_client->state(), client::ClientState::kClosed);
    const auto disconnects = packetsOf<protocol::Disconnect>(_client->takeDatagrams(_clock.now()));
    ASSERT_EQ(disconnects.size(), 3U);
    for (const auto& disconnect : disconnects) {
        EXPECT_EQ(disconnect.reason, network::DisconnectReason::kGraceful);
    }
    EXPECT_TRUE(_client->takeEvents().empty());
    EXPECT_FALSE(_client->send(network::Channel::kReliable, message(kGameType)));
}

TEST_F(ClientCoreTest, DisconnectTellsTheReasonItWasGiven) {
    connect();
    _client->disconnect(_clock.now(), network::DisconnectReason::kManifestMismatch);
    const auto disconnects = packetsOf<protocol::Disconnect>(_client->takeDatagrams(_clock.now()));
    ASSERT_EQ(disconnects.size(), 3U);
    for (const auto& disconnect : disconnects) {
        EXPECT_EQ(disconnect.reason, network::DisconnectReason::kManifestMismatch);
    }
}

TEST_F(ClientCoreTest, DisconnectIsIgnoredUnlessConnected) {
    _client->disconnect(_clock.now());
    EXPECT_EQ(_client->state(), client::ClientState::kIdle);
    _client->connect(_clock.now());
    (void)_client->takeDatagrams(_clock.now());
    _client->disconnect(_clock.now());
    EXPECT_EQ(_client->state(), client::ClientState::kRequesting);
    EXPECT_TRUE(_client->takeDatagrams(_clock.now()).empty());
}

TEST_F(ClientCoreTest, PacketsThatDoNotFitTheStateAreDropped) {
    receiveFromServer(protocol::PacketCodec::encode(protocol::Challenge{.challenge = 1}));
    receiveFromServer(protocol::PacketCodec::encode(protocol::Accepted{.sessionToken = 1, .client = 2}));
    receiveFromServer(protocol::PacketCodec::encode(protocol::Rejected{.reason = network::RejectReason::kServerFull}));
    EXPECT_EQ(_client->state(), client::ClientState::kIdle);

    _client->connect(_clock.now());
    receiveFromServer(protocol::PacketCodec::encode(protocol::Accepted{.sessionToken = 1, .client = 2}));
    EXPECT_EQ(_client->state(), client::ClientState::kRequesting);
    EXPECT_TRUE(_client->takeEvents().empty());
}

TEST_F(ClientCoreTest, PacketsWithAnotherTokenAreDropped) {
    connect();
    _clientEvents.clear();
    receiveFromServer(protocol::PacketCodec::encode(
        protocol::Disconnect{.sessionToken = 0xDEAD, .reason = network::DisconnectReason::kKicked}));
    EXPECT_EQ(_client->state(), client::ClientState::kConnected);
    EXPECT_TRUE(_client->takeEvents().empty());
}

TEST_F(ClientCoreTest, ServerDisconnectWithOurTokenClosesTheClient) {
    connect();
    _server->execute(server::KickClient{.client = _client->clientId(), .reason = network::DisconnectReason::kKicked});
    run(std::chrono::milliseconds{100});
    EXPECT_EQ(_client->state(), client::ClientState::kClosed);
    const auto disconnected = eventsOf<client::Disconnected>(_clientEvents);
    ASSERT_EQ(disconnected.size(), 1U);
    EXPECT_EQ(disconnected.front().reason, network::DisconnectReason::kKicked);
}

TEST_F(ClientCoreTest, GarbageNeverCrashesNorChangesTheState) {
    connect();
    network::Bytes noise(64);
    for (std::size_t length = 0; length < noise.size(); ++length) {
        for (std::size_t i = 0; i < length; ++i) {
            noise[i] = static_cast<std::byte>((i * 31 + length * 7) & 0xFF);
        }
        _client->receive(std::span<const std::byte>{noise.data(), length}, _clock.now());
    }
    _client->receive({}, _clock.now());
    EXPECT_EQ(_client->state(), client::ClientState::kConnected);
}
