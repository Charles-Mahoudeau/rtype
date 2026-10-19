/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ServerCoreTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "FakeClock.hpp"
#include "rtype/network/NetworkSettings.hpp"
#include "rtype/network/RandomSource.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/connection/Connection.hpp"
#include "rtype/network/io/Address.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/Packets.hpp"
#include "rtype/network/server/RoomMessages.hpp"
#include "rtype/network/server/ServerCore.hpp"

namespace network = rtype::network;
namespace protocol = rtype::network::protocol;
namespace server = rtype::network::server;
namespace tests = rtype::network::tests;

namespace {
constexpr std::uint8_t kReadyType = 0x09;
constexpr std::uint8_t kGameType = 0x20;

network::io::Address makeAddress(const std::uint8_t last, const std::uint16_t port = 5000) {
    network::io::Address::Bytes16 bytes{};
    bytes[10] = 0xFF;
    bytes[11] = 0xFF;
    bytes[12] = 10;
    bytes[15] = last;
    return network::io::Address{bytes, port};
}

network::Bytes message(const std::uint8_t type, const std::uint8_t fill = 7) {
    return network::Bytes{static_cast<std::byte>(type), static_cast<std::byte>(fill)};
}

/// @brief The packets of `datagrams` that are a `P`, decoded.
template <typename P>
std::vector<P> packetsOf(const std::vector<server::OutgoingDatagram>& datagrams, const network::io::Address& to) {
    std::vector<P> result;
    for (const auto& datagram : datagrams) {
        if (!(datagram.to == to)) {
            continue;
        }
        const auto packet = protocol::PacketCodec::decode(datagram.bytes);
        if (packet && std::holds_alternative<P>(*packet)) {
            result.push_back(std::get<P>(*packet));
        }
    }
    return result;
}

/// @brief What a client knows after a handshake.
struct Handshake {
    std::uint32_t token{0};
    network::ClientId client{network::kNoClient};
};

class ServerCoreTest : public ::testing::Test {
  protected:
    ServerCoreTest() : _core{makeCore(_settings)} {}

    static server::ServerCore makeCore(const network::NetworkSettings& settings) {
        std::uint64_t counter = 1000;
        return server::ServerCore{settings, [counter]() mutable { return counter++; }};
    }

    void send(const network::io::Address& from, const network::Bytes& datagram) {
        _core.receive(from, datagram, _clock.now());
    }

    std::vector<server::OutgoingDatagram> flush() { return _core.takeDatagrams(_clock.now()); }

    void sendConnectRequest(const network::io::Address& from,
                            const std::uint16_t version = protocol::kProtocolVersion) {
        send(from, protocol::PacketCodec::encode(protocol::ConnectRequest{.version = version}));
    }

    std::optional<protocol::Challenge> requestChallenge(const network::io::Address& from) {
        sendConnectRequest(from);
        const auto challenges = packetsOf<protocol::Challenge>(flush(), from);
        if (challenges.empty()) {
            return std::nullopt;
        }
        return challenges.front();
    }

    void sendChallengeResponse(const network::io::Address& from, const std::uint64_t value) {
        send(from, protocol::PacketCodec::encode(protocol::ChallengeResponse{.challenge = value}));
    }

    std::optional<Handshake> connect(const network::io::Address& from) {
        const auto challenge = requestChallenge(from);
        if (!challenge) {
            return std::nullopt;
        }
        sendChallengeResponse(from, challenge->challenge);
        const auto accepted = packetsOf<protocol::Accepted>(flush(), from);
        if (accepted.empty()) {
            return std::nullopt;
        }
        return Handshake{accepted.front().sessionToken, accepted.front().client};
    }

    /// @brief Sends `payload` reliably from a client holding `handshake`.
    void sendFromClient(const network::io::Address& from, const Handshake& handshake, const network::Bytes& payload,
                        const network::Channel channel = network::Channel::kReliable) {
        network::connection::Connection client{handshake.token, _settings, _clock.now()};
        client.send(channel, payload);
        for (const auto& datagram : client.buildPackets(_clock.now())) {
            send(from, datagram);
        }
    }

    /// @brief Connects and answers `kReady`; the `ClientJoined` event is consumed.
    std::optional<Handshake> join(const network::io::Address& from) {
        const auto handshake = connect(from);
        if (handshake) {
            sendFromClient(from, *handshake, message(kReadyType));
            static_cast<void>(_core.takeEvents());
        }
        return handshake;
    }

    network::NetworkSettings _settings;
    tests::FakeClock _clock;
    server::ServerCore _core;
};
}  // namespace

TEST_F(ServerCoreTest, FullHandshakeEndsWithClientJoined) {
    const auto address = makeAddress(1);
    const auto handshake = connect(address);

    ASSERT_TRUE(handshake);
    EXPECT_EQ(handshake->client, 1U);
    EXPECT_NE(handshake->token, 0U);
    EXPECT_TRUE(_core.takeEvents().empty());

    sendFromClient(address, *handshake, message(kReadyType));
    const auto events = _core.takeEvents();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<server::ClientJoined>(events.front()));
    EXPECT_EQ(std::get<server::ClientJoined>(events.front()).client, 1U);
}

TEST_F(ServerCoreTest, WelcomeMessagesAreSentAfterAccepted) {
    _core.setWelcomeMessages({message(0x21), message(0x22)});
    const auto address = makeAddress(1);
    const auto challenge = requestChallenge(address);
    ASSERT_TRUE(challenge);

    sendChallengeResponse(address, challenge->challenge);
    const auto datagrams = flush();

    EXPECT_EQ(packetsOf<protocol::Accepted>(datagrams, address).size(), 1U);
    EXPECT_FALSE(packetsOf<protocol::Data>(datagrams, address).empty());
}

TEST_F(ServerCoreTest, WrongChallengeValueIsDroppedSilently) {
    const auto address = makeAddress(1);
    const auto challenge = requestChallenge(address);
    ASSERT_TRUE(challenge);

    sendChallengeResponse(address, challenge->challenge + 1);

    EXPECT_TRUE(flush().empty());
    EXPECT_EQ(_core.sessionCount(), 0U);
}

TEST_F(ServerCoreTest, ChallengeEchoedFromAnotherAddressIsDropped) {
    const auto victim = makeAddress(1);
    const auto attacker = makeAddress(2);
    const auto challenge = requestChallenge(victim);
    ASSERT_TRUE(challenge);

    sendChallengeResponse(attacker, challenge->challenge);

    EXPECT_TRUE(flush().empty());
    EXPECT_EQ(_core.sessionCount(), 0U);
}

TEST_F(ServerCoreTest, ExpiredChallengeIsDropped) {
    const auto address = makeAddress(1);
    const auto challenge = requestChallenge(address);
    ASSERT_TRUE(challenge);

    _clock.advance(_settings.challengeExpiry + std::chrono::milliseconds{1});
    sendChallengeResponse(address, challenge->challenge);

    EXPECT_TRUE(flush().empty());
    EXPECT_EQ(_core.sessionCount(), 0U);
}

TEST_F(ServerCoreTest, UpdateRemovesExpiredChallenges) {
    ASSERT_TRUE(requestChallenge(makeAddress(1)));
    EXPECT_EQ(_core.pendingChallengeCount(), 1U);

    _clock.advance(_settings.challengeExpiry + std::chrono::milliseconds{1});
    _core.update(_clock.now());

    EXPECT_EQ(_core.pendingChallengeCount(), 0U);
}

TEST_F(ServerCoreTest, VersionMismatchIsRejected) {
    const auto address = makeAddress(1);
    sendConnectRequest(address, protocol::kProtocolVersion + 1);

    const auto rejected = packetsOf<protocol::Rejected>(flush(), address);

    ASSERT_EQ(rejected.size(), 1U);
    EXPECT_EQ(rejected.front().reason, network::RejectReason::kVersionMismatch);
    EXPECT_EQ(_core.pendingChallengeCount(), 0U);
}

TEST_F(ServerCoreTest, ServerFullRejectsTheRequest) {
    _settings.maxClients = 1;
    _core = makeCore(_settings);
    ASSERT_TRUE(connect(makeAddress(1)));

    const auto address = makeAddress(2);
    sendConnectRequest(address);
    const auto rejected = packetsOf<protocol::Rejected>(flush(), address);

    ASSERT_EQ(rejected.size(), 1U);
    EXPECT_EQ(rejected.front().reason, network::RejectReason::kServerFull);
}

TEST_F(ServerCoreTest, ServerFullRejectsTheResponse) {
    _settings.maxClients = 1;
    _core = makeCore(_settings);
    const auto late = makeAddress(2);
    const auto challenge = requestChallenge(late);
    ASSERT_TRUE(challenge);
    ASSERT_TRUE(connect(makeAddress(1)));

    sendChallengeResponse(late, challenge->challenge);
    const auto rejected = packetsOf<protocol::Rejected>(flush(), late);

    ASSERT_EQ(rejected.size(), 1U);
    EXPECT_EQ(rejected.front().reason, network::RejectReason::kServerFull);
    EXPECT_EQ(_core.sessionCount(), 1U);
}

TEST_F(ServerCoreTest, RepeatedConnectRequestGetsTheSameChallenge) {
    const auto address = makeAddress(1);
    const auto first = requestChallenge(address);
    const auto second = requestChallenge(address);

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_EQ(first->challenge, second->challenge);
    EXPECT_EQ(_core.pendingChallengeCount(), 1U);
}

TEST_F(ServerCoreTest, RepeatedChallengeResponseGetsTheSameAccepted) {
    const auto address = makeAddress(1);
    const auto challenge = requestChallenge(address);
    ASSERT_TRUE(challenge);
    sendChallengeResponse(address, challenge->challenge);
    const auto first = packetsOf<protocol::Accepted>(flush(), address);

    sendChallengeResponse(address, challenge->challenge);
    const auto second = packetsOf<protocol::Accepted>(flush(), address);

    ASSERT_EQ(first.size(), 1U);
    ASSERT_EQ(second.size(), 1U);
    EXPECT_EQ(first.front().sessionToken, second.front().sessionToken);
    EXPECT_EQ(first.front().client, second.front().client);
    EXPECT_EQ(first.front().resumeToken, second.front().resumeToken);
    EXPECT_EQ(_core.sessionCount(), 1U);
}

TEST_F(ServerCoreTest, DataWithAWrongTokenIsDropped) {
    const auto address = makeAddress(1);
    const auto handshake = connect(address);
    ASSERT_TRUE(handshake);

    sendFromClient(address, Handshake{handshake->token + 1, handshake->client}, message(kReadyType));

    EXPECT_TRUE(_core.takeEvents().empty());
}

TEST_F(ServerCoreTest, DataFromAnotherAddressIsDropped) {
    const auto handshake = connect(makeAddress(1));
    ASSERT_TRUE(handshake);

    sendFromClient(makeAddress(2), *handshake, message(kReadyType));

    EXPECT_TRUE(_core.takeEvents().empty());
}

TEST_F(ServerCoreTest, MalformedDataIsDroppedAndCounted) {
    const auto address = makeAddress(1);
    const auto handshake = connect(address);
    ASSERT_TRUE(handshake);
    // A reliable message announcing 5 bytes but carrying none.
    network::Bytes messages{std::byte{1}, std::byte{0}, std::byte{0}, std::byte{0}, std::byte{5}};
    send(address,
         protocol::PacketCodec::encode(protocol::Data{.sessionToken = handshake->token, .messages = messages}));

    EXPECT_EQ(_core.malformedPacketCount(), 1U);
    EXPECT_TRUE(_core.takeEvents().empty());
}

TEST_F(ServerCoreTest, GarbageIsDropped) {
    send(makeAddress(1), network::Bytes{});
    send(makeAddress(1), network::Bytes(40, std::byte{0xFF}));

    EXPECT_TRUE(flush().empty());
    EXPECT_TRUE(_core.takeEvents().empty());
}

TEST_F(ServerCoreTest, MessagesBeforeReadyAreDropped) {
    const auto address = makeAddress(1);
    const auto handshake = connect(address);
    ASSERT_TRUE(handshake);

    sendFromClient(address, *handshake, message(kGameType));

    EXPECT_TRUE(_core.takeEvents().empty());
    EXPECT_EQ(_core.unexpectedMessageCount(), 1U);
}

TEST_F(ServerCoreTest, GameMessagesAreEmittedAfterReady) {
    const auto address = makeAddress(1);
    const auto handshake = join(address);
    ASSERT_TRUE(handshake);

    sendFromClient(address, *handshake, message(kGameType, 9), network::Channel::kUnreliable);
    const auto events = _core.takeEvents();

    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<server::ClientMessage>(events.front()));
    const auto& received = std::get<server::ClientMessage>(events.front());
    EXPECT_EQ(received.client, handshake->client);
    EXPECT_EQ(received.channel, network::Channel::kUnreliable);
    EXPECT_EQ(received.payload, message(kGameType, 9));
}

TEST_F(ServerCoreTest, SecondReadyIsIgnoredAndOtherTransportTypesAreCounted) {
    const auto address = makeAddress(1);
    const auto handshake = connect(address);
    ASSERT_TRUE(handshake);
    network::connection::Connection client{handshake->token, _settings, _clock.now()};
    client.send(network::Channel::kReliable, message(kReadyType));
    client.send(network::Channel::kReliable, message(kReadyType));
    client.send(network::Channel::kReliable, message(0x01));
    for (const auto& datagram : client.buildPackets(_clock.now())) {
        send(address, datagram);
    }
    const auto events = _core.takeEvents();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_TRUE(std::holds_alternative<server::ClientJoined>(events.front()));
    EXPECT_EQ(_core.unexpectedMessageCount(), 1U);
}

TEST_F(ServerCoreTest, SilentSessionTimesOut) {
    const auto handshake = join(makeAddress(1));
    ASSERT_TRUE(handshake);

    _clock.advance(_settings.timeout - std::chrono::milliseconds{1});
    _core.update(_clock.now());
    EXPECT_EQ(_core.sessionCount(), 1U);

    _clock.advance(std::chrono::milliseconds{2});
    _core.update(_clock.now());
    const auto events = _core.takeEvents();

    EXPECT_EQ(_core.sessionCount(), 0U);
    ASSERT_EQ(events.size(), 1U);
    ASSERT_TRUE(std::holds_alternative<server::ClientLeft>(events.front()));
    EXPECT_EQ(std::get<server::ClientLeft>(events.front()).client, handshake->client);
    EXPECT_EQ(std::get<server::ClientLeft>(events.front()).reason, network::DisconnectReason::kTimeout);
}

TEST_F(ServerCoreTest, GracefulDisconnectClosesTheSession) {
    const auto address = makeAddress(1);
    const auto handshake = join(address);
    ASSERT_TRUE(handshake);

    send(address, protocol::PacketCodec::encode(protocol::Disconnect{.sessionToken = handshake->token,
                                                                     .reason = network::DisconnectReason::kGraceful}));
    const auto events = _core.takeEvents();

    EXPECT_EQ(_core.sessionCount(), 0U);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(std::get<server::ClientLeft>(events.front()).reason, network::DisconnectReason::kGraceful);
}

TEST_F(ServerCoreTest, DisconnectReasonSentByTheClientIsReportedAsGraceful) {
    const auto address = makeAddress(1);
    const auto handshake = join(address);
    ASSERT_TRUE(handshake);

    send(address, protocol::PacketCodec::encode(protocol::Disconnect{.sessionToken = handshake->token,
                                                                     .reason = network::DisconnectReason::kKicked}));
    const auto events = _core.takeEvents();

    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(std::get<server::ClientLeft>(events.front()).reason, network::DisconnectReason::kGraceful);
}

TEST_F(ServerCoreTest, DisconnectWithAWrongTokenIsDropped) {
    const auto address = makeAddress(1);
    const auto handshake = join(address);
    ASSERT_TRUE(handshake);

    send(address, protocol::PacketCodec::encode(protocol::Disconnect{.sessionToken = handshake->token + 1,
                                                                     .reason = network::DisconnectReason::kGraceful}));

    EXPECT_EQ(_core.sessionCount(), 1U);
}

TEST_F(ServerCoreTest, KickSendsDisconnectThreeTimesThenCloses) {
    const auto address = makeAddress(1);
    const auto handshake = join(address);
    ASSERT_TRUE(handshake);
    static_cast<void>(flush());

    _core.execute(server::KickClient{.client = handshake->client, .reason = network::DisconnectReason::kKicked});
    const auto disconnects = packetsOf<protocol::Disconnect>(flush(), address);
    const auto events = _core.takeEvents();

    ASSERT_EQ(disconnects.size(), 3U);
    EXPECT_EQ(disconnects.front().sessionToken, handshake->token);
    EXPECT_EQ(disconnects.front().reason, network::DisconnectReason::kKicked);
    EXPECT_EQ(_core.sessionCount(), 0U);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(std::get<server::ClientLeft>(events.front()).reason, network::DisconnectReason::kKicked);
}

TEST_F(ServerCoreTest, OverflowedSessionIsDisconnected) {
    const auto address = makeAddress(1);
    const auto handshake = join(address);
    ASSERT_TRUE(handshake);
    static_cast<void>(flush());

    const network::Bytes payload = message(kGameType);
    for (std::size_t i = 0; i <= protocol::kMaxPendingReliable + 1; ++i) {
        _core.execute(server::SendMessage{
            .client = handshake->client, .channel = network::Channel::kReliable, .payload = payload});
    }
    _core.update(_clock.now());
    const auto disconnects = packetsOf<protocol::Disconnect>(flush(), address);
    const auto events = _core.takeEvents();

    ASSERT_EQ(disconnects.size(), 3U);
    EXPECT_EQ(disconnects.front().reason, network::DisconnectReason::kOverflow);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(std::get<server::ClientLeft>(events.front()).reason, network::DisconnectReason::kOverflow);
}

TEST_F(ServerCoreTest, ShutdownDisconnectsEveryoneAndIgnoresDatagrams) {
    const auto first = makeAddress(1);
    const auto second = makeAddress(2);
    ASSERT_TRUE(join(first));
    ASSERT_TRUE(join(second));
    static_cast<void>(flush());

    _core.shutdown(_clock.now());
    const auto datagrams = flush();
    const auto events = _core.takeEvents();

    EXPECT_EQ(packetsOf<protocol::Disconnect>(datagrams, first).size(), 3U);
    EXPECT_EQ(packetsOf<protocol::Disconnect>(datagrams, second).size(), 3U);
    EXPECT_EQ(packetsOf<protocol::Disconnect>(datagrams, first).front().reason,
              network::DisconnectReason::kServerShutdown);
    EXPECT_EQ(_core.sessionCount(), 0U);
    EXPECT_EQ(events.size(), 2U);

    sendConnectRequest(makeAddress(3));
    EXPECT_TRUE(flush().empty());
}

TEST_F(ServerCoreTest, SendMessageReachesOneClientAndAllClientsReachesReadySessions) {
    const auto ready = makeAddress(1);
    const auto welcoming = makeAddress(2);
    const auto readyHandshake = join(ready);
    ASSERT_TRUE(readyHandshake);
    ASSERT_TRUE(connect(welcoming));
    static_cast<void>(flush());

    _core.execute(server::SendMessage{
        .client = server::kAllClients, .channel = network::Channel::kReliable, .payload = message(kGameType)});
    const auto datagrams = flush();
    EXPECT_FALSE(packetsOf<protocol::Data>(datagrams, ready).empty());
    EXPECT_TRUE(packetsOf<protocol::Data>(datagrams, welcoming).empty());

    _core.execute(
        server::SendMessage{.client = 999, .channel = network::Channel::kReliable, .payload = message(kGameType)});
    EXPECT_NO_FATAL_FAILURE(static_cast<void>(flush()));
}

TEST_F(ServerCoreTest, SendMessageToAWelcomingClientIsIgnored) {
    const auto address = makeAddress(1);
    const auto handshake = connect(address);
    ASSERT_TRUE(handshake);
    static_cast<void>(flush());

    _core.execute(server::SendMessage{
        .client = handshake->client, .channel = network::Channel::kReliable, .payload = message(kGameType)});

    EXPECT_TRUE(packetsOf<protocol::Data>(flush(), address).empty());
}

TEST_F(ServerCoreTest, ClientLeftIsNotEmittedForASessionThatNeverSentReady) {
    const auto handshake = connect(makeAddress(1));
    ASSERT_TRUE(handshake);

    _clock.advance(_settings.timeout + std::chrono::milliseconds{1});
    _core.update(_clock.now());

    EXPECT_EQ(_core.sessionCount(), 0U);
    EXPECT_TRUE(_core.takeEvents().empty());
}

TEST_F(ServerCoreTest, ClosedClientIdIsNeverGivenAgain) {
    const auto address = makeAddress(1);
    const auto first = join(address);
    ASSERT_TRUE(first);
    send(address, protocol::PacketCodec::encode(protocol::Disconnect{.sessionToken = first->token}));

    const auto second = connect(address);

    ASSERT_TRUE(second);
    EXPECT_EQ(first->client, 1U);
    EXPECT_EQ(second->client, 2U);
}

TEST_F(ServerCoreTest, NewConnectionFromAnAddressWithASessionReplacesIt) {
    const auto address = makeAddress(1);
    const auto first = join(address);
    ASSERT_TRUE(first);

    const auto second = connect(address);
    const auto events = _core.takeEvents();

    ASSERT_TRUE(second);
    EXPECT_NE(second->client, first->client);
    EXPECT_EQ(_core.sessionCount(), 1U);
    ASSERT_EQ(events.size(), 1U);
    EXPECT_EQ(std::get<server::ClientLeft>(events.front()).client, first->client);
    EXPECT_EQ(std::get<server::ClientLeft>(events.front()).reason, network::DisconnectReason::kGraceful);

    sendFromClient(address, *first, message(kGameType));
    EXPECT_TRUE(_core.takeEvents().empty());
}

TEST_F(ServerCoreTest, ReplacementIsAllowedWhenTheServerIsFull) {
    _settings.maxClients = 1;
    _core = makeCore(_settings);
    const auto address = makeAddress(1);
    ASSERT_TRUE(connect(address));

    EXPECT_TRUE(connect(address));
    EXPECT_EQ(_core.sessionCount(), 1U);
}

TEST_F(ServerCoreTest, OldestChallengeIsDroppedAtTheLimit) {
    std::vector<std::uint64_t> values;
    for (std::size_t i = 0; i < _settings.maxPendingChallenges + 1; ++i) {
        const auto challenge =
            requestChallenge(makeAddress(static_cast<std::uint8_t>(i), static_cast<std::uint16_t>(6000 + i)));
        ASSERT_TRUE(challenge);
        values.push_back(challenge->challenge);
    }

    EXPECT_EQ(_core.pendingChallengeCount(), _settings.maxPendingChallenges);

    sendChallengeResponse(makeAddress(0, 6000), values.front());
    EXPECT_EQ(_core.sessionCount(), 0U);

    const std::size_t last = _settings.maxPendingChallenges;
    sendChallengeResponse(makeAddress(static_cast<std::uint8_t>(last), static_cast<std::uint16_t>(6000 + last)),
                          values.back());
    EXPECT_EQ(_core.sessionCount(), 1U);
}

TEST(ServerCoreDeterminism, SameInputsGiveTheSameOutput) {
    const auto run = [] {
        tests::FakeClock clock;
        std::uint64_t counter = 42;
        server::ServerCore core{network::NetworkSettings{}, [&counter] { return counter++; }};
        const auto address = makeAddress(1);
        core.receive(address,
                     protocol::PacketCodec::encode(protocol::ConnectRequest{.version = protocol::kProtocolVersion}),
                     clock.now());
        std::vector<network::Bytes> out;
        for (auto& datagram : core.takeDatagrams(clock.now())) {
            out.push_back(std::move(datagram.bytes));
        }
        return out;
    };

    EXPECT_EQ(run(), run());
}
