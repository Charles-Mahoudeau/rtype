/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PacketsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/bytes/Overloaded.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/Packets.hpp"

namespace network = rtype::network;
namespace protocol = rtype::network::protocol;

namespace {
std::vector<std::byte> toBytes(const std::vector<int>& values) {
    std::vector<std::byte> result;
    for (const int value : values) {
        result.push_back(static_cast<std::byte>(value));
    }
    return result;
}

template <typename P>
std::optional<P> roundTrip(const P& packet) {
    const network::Bytes encoded = protocol::PacketCodec::encode(packet);
    const std::optional<protocol::Packet> decoded = protocol::PacketCodec::decode(encoded);
    if (!decoded || !std::holds_alternative<P>(*decoded)) {
        return std::nullopt;
    }
    return std::get<P>(*decoded);
}

/// @brief A valid 64-byte connect request, to be altered by the tests.
network::Bytes validConnectRequest() { return protocol::PacketCodec::encode(protocol::ConnectRequest{.version = 1}); }
}  // namespace

TEST(Packets, ConnectRequestRoundTripsAndIs64Bytes) {
    const protocol::ConnectRequest packet{.version = 3, .resumeClient = 7, .resumeToken = 0x1122334455667788ULL};

    EXPECT_EQ(protocol::PacketCodec::encode(packet).size(), 64U);
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->version, 3);
    EXPECT_EQ(decoded->resumeClient, 7U);
    EXPECT_EQ(decoded->resumeToken, 0x1122334455667788ULL);
}

TEST(Packets, ConnectRequestLayout) {
    const network::Bytes encoded = protocol::PacketCodec::encode(
        protocol::ConnectRequest{.version = 0x0102, .resumeClient = 0x0A0B0C0D, .resumeToken = 1});

    ASSERT_EQ(encoded.size(), 64U);
    const std::vector<std::byte> head =
        toBytes({0x01, 0x50, 0x59, 0x54, 0x52, 0x02, 0x01, 0x0D, 0x0C, 0x0B, 0x0A, 1, 0, 0, 0, 0, 0, 0, 0});
    EXPECT_TRUE(std::equal(head.begin(), head.end(), encoded.begin()));
    for (std::size_t index = head.size(); index < encoded.size(); ++index) {
        EXPECT_EQ(encoded[index], std::byte{0}) << index;
    }
}

TEST(Packets, ChallengeRoundTripsAndIs9Bytes) {
    const protocol::Challenge packet{.challenge = 0xAABBCCDD11223344ULL};

    EXPECT_EQ(protocol::PacketCodec::encode(packet).size(), 9U);
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->challenge, packet.challenge);
}

TEST(Packets, ChallengeResponseRoundTripsAndIs64Bytes) {
    const protocol::ChallengeResponse packet{.challenge = 99};

    EXPECT_EQ(protocol::PacketCodec::encode(packet).size(), 64U);
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->challenge, 99U);
}

TEST(Packets, AcceptedRoundTripsAndIs17Bytes) {
    const protocol::Accepted packet{.sessionToken = 0xA1B2C3D4, .client = 7, .resumeToken = 1};
    const network::Bytes encoded = protocol::PacketCodec::encode(packet);

    EXPECT_EQ(encoded, toBytes({0x04, 0xD4, 0xC3, 0xB2, 0xA1, 7, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0}));
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->sessionToken, 0xA1B2C3D4U);
    EXPECT_EQ(decoded->client, 7U);
    EXPECT_EQ(decoded->resumeToken, 1U);
}

TEST(Packets, RejectedRoundTripsAndIs2Bytes) {
    const protocol::Rejected packet{.reason = network::RejectReason::kServerFull};

    EXPECT_EQ(protocol::PacketCodec::encode(packet), toBytes({0x05, 2}));
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->reason, network::RejectReason::kServerFull);
}

TEST(Packets, RejectedRefusesAReasonOutOfRangeAndZero) {
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x05, 0})));
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x05, 4})));
    EXPECT_TRUE(protocol::PacketCodec::decode(toBytes({0x05, 3})));
}

TEST(Packets, DataRoundTripsAndPointsIntoTheDatagram) {
    const std::vector<std::byte> payload = toBytes({1, 2, 3, 4, 5});
    const network::Bytes encoded = protocol::PacketCodec::encode(protocol::Data{
        .sessionToken = 0x01020304, .sequence = 0x0506, .ack = 0x0708, .ackBits = 0x090A0B0C, .messages = payload});

    EXPECT_EQ(encoded.size(), 13U + payload.size());
    EXPECT_EQ(std::vector<std::byte>(encoded.begin(), encoded.begin() + 13),
              toBytes({0x06, 4, 3, 2, 1, 6, 5, 8, 7, 0x0C, 0x0B, 0x0A, 9}));
    const std::optional<protocol::Packet> decoded = protocol::PacketCodec::decode(encoded);
    ASSERT_TRUE(decoded);
    const auto& data = std::get<protocol::Data>(*decoded);
    EXPECT_EQ(data.sessionToken, 0x01020304U);
    EXPECT_EQ(data.sequence, 0x0506);
    EXPECT_EQ(data.ack, 0x0708);
    EXPECT_EQ(data.ackBits, 0x090A0B0CU);
    ASSERT_EQ(data.messages.size(), payload.size());
    EXPECT_EQ(data.messages.data(), encoded.data() + 13);
}

TEST(Packets, DataWithoutMessagesIs13Bytes) {
    const network::Bytes encoded = protocol::PacketCodec::encode(protocol::Data{.sessionToken = 1});

    EXPECT_EQ(encoded.size(), 13U);
    const auto decoded = roundTrip(protocol::Data{.sessionToken = 1});
    ASSERT_TRUE(decoded);
    EXPECT_TRUE(decoded->messages.empty());
}

TEST(Packets, DataTruncatedInItsHeaderIsRejected) {
    const network::Bytes encoded = protocol::PacketCodec::encode(protocol::Data{});

    for (std::size_t size = 0; size < encoded.size(); ++size) {
        EXPECT_FALSE(protocol::PacketCodec::decode(std::span{encoded}.first(size))) << size;
    }
}

TEST(Packets, DisconnectRoundTripsAndIs6Bytes) {
    const protocol::Disconnect packet{.sessionToken = 0xCAFEBABE, .reason = network::DisconnectReason::kTimeout};
    const network::Bytes encoded = protocol::PacketCodec::encode(packet);

    EXPECT_EQ(encoded, toBytes({0x07, 0xBE, 0xBA, 0xFE, 0xCA, 2}));
    const auto decoded = roundTrip(packet);
    ASSERT_TRUE(decoded);
    EXPECT_EQ(decoded->sessionToken, 0xCAFEBABEU);
    EXPECT_EQ(decoded->reason, network::DisconnectReason::kTimeout);
}

TEST(Packets, DisconnectRefusesAReasonOutOfRangeAndZero) {
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x07, 0, 0, 0, 0, 0})));
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x07, 0, 0, 0, 0, 8})));
    EXPECT_TRUE(protocol::PacketCodec::decode(toBytes({0x07, 0, 0, 0, 0, 7})));
}

TEST(Packets, ConnectRequestWithAWrongProtocolIdIsRejected) {
    for (std::size_t index = 1; index <= 4; ++index) {
        network::Bytes encoded = validConnectRequest();
        encoded[index] ^= std::byte{0x01};
        EXPECT_FALSE(protocol::PacketCodec::decode(encoded)) << index;
    }
    EXPECT_TRUE(protocol::PacketCodec::decode(validConnectRequest()));
}

TEST(Packets, PaddedPacketsOf63And65BytesAreRejected) {
    network::Bytes connect = validConnectRequest();
    connect.pop_back();
    EXPECT_FALSE(protocol::PacketCodec::decode(connect));
    connect.push_back(std::byte{0});
    connect.push_back(std::byte{0});
    EXPECT_FALSE(protocol::PacketCodec::decode(connect));

    network::Bytes response = protocol::PacketCodec::encode(protocol::ChallengeResponse{.challenge = 1});
    ASSERT_EQ(response.size(), 64U);
    response.pop_back();
    EXPECT_FALSE(protocol::PacketCodec::decode(response));
    response.push_back(std::byte{0});
    response.push_back(std::byte{0});
    EXPECT_FALSE(protocol::PacketCodec::decode(response));
}

TEST(Packets, FixedSizePacketsRejectBytesLeftOver) {
    network::Bytes challenge = protocol::PacketCodec::encode(protocol::Challenge{.challenge = 1});
    challenge.push_back(std::byte{0});
    EXPECT_FALSE(protocol::PacketCodec::decode(challenge));

    network::Bytes accepted = protocol::PacketCodec::encode(protocol::Accepted{});
    accepted.push_back(std::byte{0});
    EXPECT_FALSE(protocol::PacketCodec::decode(accepted));
}

TEST(Packets, KindZeroAndKindEightAreUnknown) {
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x00})));
    EXPECT_FALSE(protocol::PacketCodec::decode(toBytes({0x08, 1, 2, 3})));
    EXPECT_FALSE(protocol::PacketCodec::peekKind(toBytes({0x00})));
    EXPECT_EQ(protocol::PacketCodec::peekKind(toBytes({0x04})), protocol::PacketKind::kAccepted);
}

TEST(Packets, DecodeVisitsTheMatchingAlternative) {
    const network::Bytes encoded = protocol::PacketCodec::encode(protocol::Challenge{.challenge = 5});
    const std::optional<protocol::Packet> decoded = protocol::PacketCodec::decode(encoded);

    ASSERT_TRUE(decoded);
    const std::uint64_t seen =
        std::visit(network::bytes::Overloaded{[](const protocol::Challenge& challenge) { return challenge.challenge; },
                                              [](const auto&) { return std::uint64_t{0}; }},
                   *decoded);
    EXPECT_EQ(seen, 5U);
}

TEST(Packets, EveryServerReplyIsSmallerThanTheHandshakeRequests) {
    // Anti-amplification: no answer to a 64-byte request is bigger than the request.
    constexpr std::size_t kChallengeSize = 1 + sizeof(std::uint64_t);
    constexpr std::size_t kAcceptedSize = 1 + sizeof(std::uint32_t) + sizeof(network::ClientId) + sizeof(std::uint64_t);
    constexpr std::size_t kRejectedSize = 1 + sizeof(network::RejectReason);

    static_assert(kChallengeSize < protocol::kHandshakePadding);
    static_assert(kAcceptedSize < protocol::kHandshakePadding);
    static_assert(kRejectedSize < protocol::kHandshakePadding);
    EXPECT_EQ(protocol::PacketCodec::encode(protocol::Challenge{}).size(), kChallengeSize);
    EXPECT_EQ(protocol::PacketCodec::encode(protocol::Accepted{}).size(), kAcceptedSize);
    EXPECT_EQ(protocol::PacketCodec::encode(protocol::Rejected{}).size(), kRejectedSize);
}

TEST(WireValues, ZeroIsNeverValid) {
    EXPECT_FALSE(isValidWireValue(static_cast<protocol::PacketKind>(0)));
    EXPECT_FALSE(isValidWireValue(static_cast<network::RejectReason>(0)));
    EXPECT_FALSE(isValidWireValue(static_cast<network::DisconnectReason>(0)));
    EXPECT_TRUE(isValidWireValue(network::Channel::kUnreliable));
    EXPECT_TRUE(isValidWireValue(network::Channel::kReliable));
    EXPECT_FALSE(isValidWireValue(static_cast<network::Channel>(2)));
}
