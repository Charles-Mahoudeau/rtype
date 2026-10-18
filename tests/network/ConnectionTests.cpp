/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConnectionTests
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
#include "rtype/network/connection/Connection.hpp"
#include "rtype/network/protocol/Constants.hpp"
#include "rtype/network/protocol/Packets.hpp"

namespace network = rtype::network;
namespace protocol = rtype::network::protocol;
namespace tests = rtype::network::tests;
using rtype::network::connection::Connection;

namespace {
constexpr std::uint32_t kToken = 0xCAFEBABE;
constexpr std::uint16_t kNothingAcked = 0xFFFF;

/// @brief The fields of the header of a datagram.
struct Header {
    std::uint16_t sequence{0};
    std::uint16_t ack{0};
    std::uint32_t ackBits{0};
    std::size_t messageBytes{0};
};

std::vector<std::byte> toBytes(const std::vector<int>& values) {
    std::vector<std::byte> result;
    for (const int value : values) {
        result.push_back(static_cast<std::byte>(value));
    }
    return result;
}

network::Bytes payloadOf(const std::size_t size, const int fill = 1) {
    return network::Bytes(size, static_cast<std::byte>(fill));
}

std::optional<Header> headerOf(const network::Bytes& datagram) {
    const auto decoded = protocol::PacketCodec::decode(datagram);
    if (!decoded || !std::holds_alternative<protocol::Data>(*decoded)) {
        return std::nullopt;
    }
    const auto& data = std::get<protocol::Data>(*decoded);
    return Header{data.sequence, data.ack, data.ackBits, data.messages.size()};
}

/// @brief Hands a datagram built by a connection to another one, as the core would.
bool deliver(Connection& to, const network::Bytes& datagram, const network::TimePoint now) {
    const auto decoded = protocol::PacketCodec::decode(datagram);
    if (!decoded || !std::holds_alternative<protocol::Data>(*decoded)) {
        return false;
    }
    return to.receive(std::get<protocol::Data>(*decoded), now);
}

/// @brief Sends one small message and returns the single datagram that carries it.
network::Bytes sendOne(Connection& from, const network::TimePoint now, const int fill = 1) {
    EXPECT_TRUE(from.send(network::Channel::kUnreliable, payloadOf(4, fill)));
    auto packets = from.buildPackets(now);
    EXPECT_EQ(packets.size(), 1U);
    return packets.empty() ? network::Bytes{} : packets.front();
}

/// @brief Gives the header of the datagram `connection` would send right now, with a keepalive if needed.
Header currentHeader(Connection& connection, tests::FakeClock& clock) {
    clock.advance(std::chrono::milliseconds{100});
    const auto packets = connection.buildPackets(clock.now());
    EXPECT_EQ(packets.size(), 1U);
    const auto header = packets.empty() ? std::nullopt : headerOf(packets.front());
    EXPECT_TRUE(header);
    return header.value_or(Header{});
}

/// @brief Builds a `Data` with raw message bytes.
bool receiveRaw(Connection& to, const std::vector<std::byte>& messages, const network::TimePoint now) {
    const protocol::Data data{.sessionToken = kToken, .sequence = 0, .ack = 0, .ackBits = 0, .messages = messages};
    return to.receive(data, now);
}
}  // namespace

TEST(Connection, SendRefusesEmptyOversizedAndReliablePayloads) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection connection{kToken, settings, clock.now()};

    EXPECT_FALSE(connection.send(network::Channel::kUnreliable, {}));
    EXPECT_FALSE(connection.send(network::Channel::kUnreliable, payloadOf(protocol::kMaxMessageSize + 1)));
    EXPECT_FALSE(connection.send(network::Channel::kReliable, payloadOf(4)));
    EXPECT_TRUE(connection.buildPackets(clock.now()).empty());

    EXPECT_TRUE(connection.send(network::Channel::kUnreliable, payloadOf(protocol::kMaxMessageSize)));
    EXPECT_EQ(connection.buildPackets(clock.now()).size(), 1U);
}

TEST(Connection, SmallMessagesShareOneDatagramAndArriveInOrder) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};

    ASSERT_TRUE(sender.send(network::Channel::kUnreliable, payloadOf(10, 1)));
    ASSERT_TRUE(sender.send(network::Channel::kUnreliable, payloadOf(20, 2)));
    ASSERT_TRUE(sender.send(network::Channel::kUnreliable, payloadOf(30, 3)));
    const auto packets = sender.buildPackets(clock.now());

    ASSERT_EQ(packets.size(), 1U);
    EXPECT_EQ(headerOf(packets.front())->messageBytes, 3U * 3U + 60U);
    ASSERT_TRUE(deliver(receiver, packets.front(), clock.now()));
    const auto messages = receiver.takeMessages();
    ASSERT_EQ(messages.size(), 3U);
    EXPECT_EQ(messages[0].payload, payloadOf(10, 1));
    EXPECT_EQ(messages[1].payload, payloadOf(20, 2));
    EXPECT_EQ(messages[2].payload, payloadOf(30, 3));
    EXPECT_EQ(messages[0].channel, network::Channel::kUnreliable);
    EXPECT_TRUE(receiver.takeMessages().empty());
}

TEST(Connection, MessagesAreNeverSplitBetweenDatagrams) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};

    for (int i = 1; i <= 3; ++i) {
        ASSERT_TRUE(sender.send(network::Channel::kUnreliable, payloadOf(500, i)));
    }
    const auto packets = sender.buildPackets(clock.now());

    ASSERT_EQ(packets.size(), 2U);
    for (const auto& packet : packets) {
        EXPECT_LE(packet.size(), protocol::kMaxPacketSize);
        ASSERT_TRUE(deliver(receiver, packet, clock.now()));
    }
    const auto messages = receiver.takeMessages();
    ASSERT_EQ(messages.size(), 3U);
    EXPECT_EQ(messages[2].payload, payloadOf(500, 3));
    EXPECT_EQ(headerOf(packets[0])->sequence, 0);
    EXPECT_EQ(headerOf(packets[1])->sequence, 1);
}

TEST(Connection, OverflowingUnreliableMessagesAreDroppedAndCounted) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection connection{kToken, settings, clock.now()};

    // Two payloads of 1000 bytes never share a datagram, so 20 messages need 20 datagrams.
    for (int i = 0; i < 20; ++i) {
        ASSERT_TRUE(connection.send(network::Channel::kUnreliable, payloadOf(protocol::kMaxMessageSize)));
    }
    const auto packets = connection.buildPackets(clock.now());

    EXPECT_EQ(packets.size(), protocol::kMaxDatagramsPerFlush);
    EXPECT_EQ(connection.droppedMessageCount(), 20U - protocol::kMaxDatagramsPerFlush);
    EXPECT_TRUE(connection.buildPackets(clock.now()).empty());
    EXPECT_EQ(connection.droppedMessageCount(), 20U - protocol::kMaxDatagramsPerFlush);
}

TEST(Connection, KeepaliveIsSentAfterTheIdleDelayAndNotBefore) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection connection{kToken, settings, clock.now()};

    clock.advance(std::chrono::milliseconds{99});
    EXPECT_TRUE(connection.buildPackets(clock.now()).empty());

    clock.advance(std::chrono::milliseconds{1});
    const auto packets = connection.buildPackets(clock.now());
    ASSERT_EQ(packets.size(), 1U);
    EXPECT_EQ(headerOf(packets.front())->messageBytes, 0U);
    EXPECT_TRUE(connection.buildPackets(clock.now()).empty());

    clock.advance(std::chrono::milliseconds{99});
    EXPECT_TRUE(connection.buildPackets(clock.now()).empty());
    clock.advance(std::chrono::milliseconds{1});
    EXPECT_EQ(connection.buildPackets(clock.now()).size(), 1U);
}

TEST(Connection, SendingDataResetsTheKeepaliveDelay) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection connection{kToken, settings, clock.now()};

    clock.advance(std::chrono::milliseconds{80});
    static_cast<void>(sendOne(connection, clock.now()));
    clock.advance(std::chrono::milliseconds{80});

    EXPECT_TRUE(connection.buildPackets(clock.now()).empty());
}

TEST(Connection, NothingIsAckedBeforeTheFirstDatagram) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection connection{kToken, settings, clock.now()};

    const Header header = currentHeader(connection, clock);

    EXPECT_EQ(header.ack, kNothingAcked);
    EXPECT_EQ(header.ackBits, 0U);
}

TEST(Connection, InOrderDatagramsSetOneBitEach) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};

    for (int i = 0; i < 3; ++i) {
        ASSERT_TRUE(deliver(receiver, sendOne(sender, clock.now()), clock.now()));
    }
    const Header header = currentHeader(receiver, clock);

    EXPECT_EQ(header.ack, 2);
    EXPECT_EQ(header.ackBits, 0b11U);
}

TEST(Connection, OutOfOrderDatagramsSetTheirBitWhenTheyArrive) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};
    const network::Bytes first = sendOne(sender, clock.now());
    const network::Bytes second = sendOne(sender, clock.now());
    const network::Bytes third = sendOne(sender, clock.now());

    ASSERT_TRUE(deliver(receiver, first, clock.now()));
    ASSERT_TRUE(deliver(receiver, third, clock.now()));
    EXPECT_EQ(currentHeader(receiver, clock).ackBits, 0b10U);
    ASSERT_TRUE(deliver(receiver, second, clock.now()));
    const Header header = currentHeader(receiver, clock);

    EXPECT_EQ(header.ack, 2);
    EXPECT_EQ(header.ackBits, 0b11U);
}

TEST(Connection, DuplicatedDatagramsChangeNoBitButAreDeliveredAgain) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};
    const network::Bytes first = sendOne(sender, clock.now());
    const network::Bytes second = sendOne(sender, clock.now());

    ASSERT_TRUE(deliver(receiver, first, clock.now()));
    ASSERT_TRUE(deliver(receiver, second, clock.now()));
    ASSERT_TRUE(deliver(receiver, second, clock.now()));
    ASSERT_TRUE(deliver(receiver, first, clock.now()));
    const Header header = currentHeader(receiver, clock);

    EXPECT_EQ(header.ack, 1);
    EXPECT_EQ(header.ackBits, 0b1U);
    EXPECT_EQ(receiver.takeMessages().size(), 4U);
}

TEST(Connection, LostDatagramsLeaveTheirBitClear) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};
    std::vector<network::Bytes> sent;
    for (int i = 0; i < 6; ++i) {
        sent.push_back(sendOne(sender, clock.now()));
    }

    for (const std::size_t index : {0U, 2U, 3U, 5U}) {
        ASSERT_TRUE(deliver(receiver, sent[index], clock.now()));
    }
    const Header header = currentHeader(receiver, clock);

    EXPECT_EQ(header.ack, 5);
    // Bit N is sequence 4 - N: 4 (bit 0) and 1 (bit 3) were lost.
    EXPECT_EQ(header.ackBits, 0b10110U);
}

TEST(Connection, AJumpOfMoreThan32ClearsTheBits) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};
    std::vector<network::Bytes> sent;
    for (int i = 0; i < 80; ++i) {
        sent.push_back(sendOne(sender, clock.now()));
    }

    ASSERT_TRUE(deliver(receiver, sent[0], clock.now()));
    ASSERT_TRUE(deliver(receiver, sent[1], clock.now()));
    ASSERT_TRUE(deliver(receiver, sent[34], clock.now()));  // 33 ahead of 1.
    Header header = currentHeader(receiver, clock);
    EXPECT_EQ(header.ack, 34);
    EXPECT_EQ(header.ackBits, 0U);

    ASSERT_TRUE(deliver(receiver, sent[66], clock.now()));  // 32 ahead of 34: the old ack is bit 31.
    header = currentHeader(receiver, clock);
    EXPECT_EQ(header.ack, 66);
    EXPECT_EQ(header.ackBits, 0x80000000U);

    ASSERT_TRUE(deliver(receiver, sent[67], clock.now()));
    header = currentHeader(receiver, clock);
    // The old ack (34) moved to bit 32 and is gone; 66 is bit 0.
    EXPECT_EQ(header.ackBits, 1U);
}

TEST(Connection, AnOldDatagramBeyond32DoesNotChangeTheBitsButIsDelivered) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};
    std::vector<network::Bytes> sent;
    for (int i = 0; i < 40; ++i) {
        sent.push_back(sendOne(sender, clock.now()));
    }

    ASSERT_TRUE(deliver(receiver, sent[39], clock.now()));
    ASSERT_TRUE(deliver(receiver, sent[3], clock.now()));
    const Header header = currentHeader(receiver, clock);

    EXPECT_EQ(header.ack, 39);
    EXPECT_EQ(header.ackBits, 0U);
    EXPECT_EQ(receiver.takeMessages().size(), 2U);
}

TEST(Connection, AcksWorkAcrossTheSequenceWrap) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection first{kToken, settings, clock.now()};
    Connection second{kToken, settings, clock.now()};

    constexpr std::uint32_t kRounds = 70000;
    std::uint32_t acked = 0;
    for (std::uint32_t i = 0; i < kRounds; ++i) {
        ASSERT_TRUE(deliver(second, sendOne(first, clock.now()), clock.now()));
        const network::Bytes reply = sendOne(second, clock.now());
        ASSERT_TRUE(deliver(first, reply, clock.now()));
        const auto header = headerOf(reply);
        ASSERT_TRUE(header);
        EXPECT_EQ(header->ack, static_cast<std::uint16_t>(i)) << "round " << i;
        if (i >= 32) {
            ASSERT_EQ(header->ackBits, 0xFFFFFFFFU) << "round " << i;
        }
        ++acked;
        clock.advance(std::chrono::milliseconds{1});
    }

    EXPECT_EQ(acked, kRounds);
    // Every datagram was acked, so each sample was 1 ms or less: the RTT left its initial 100 ms far behind.
    EXPECT_LT(first.rtt().count(), 1.5F);
}

TEST(Connection, RttStartsAtTheInitialValueAndConvergesToTwiceTheDelay) {
    network::NetworkSettings settings;
    settings.initialRtt = std::chrono::milliseconds{500};
    tests::FakeClock clock;
    tests::SimulatedLink link{tests::LinkConditions{.delay = std::chrono::milliseconds{25}}, 1};
    Connection first{kToken, settings, clock.now()};
    Connection second{kToken, settings, clock.now()};
    EXPECT_FLOAT_EQ(first.rtt().count(), 500.0F);

    for (int step = 0; step < 400; ++step) {
        static_cast<void>(first.send(network::Channel::kUnreliable, payloadOf(4)));
        static_cast<void>(second.send(network::Channel::kUnreliable, payloadOf(4)));
        for (const auto& datagram : link.receive(0, clock.now())) {
            ASSERT_TRUE(deliver(first, datagram, clock.now()));
        }
        for (const auto& datagram : link.receive(1, clock.now())) {
            ASSERT_TRUE(deliver(second, datagram, clock.now()));
        }
        for (auto& datagram : first.buildPackets(clock.now())) {
            link.send(1, std::move(datagram), clock.now());
        }
        for (auto& datagram : second.buildPackets(clock.now())) {
            link.send(0, std::move(datagram), clock.now());
        }
        clock.advance(std::chrono::milliseconds{1});
    }

    EXPECT_NEAR(first.rtt().count(), 50.0F, 5.0F);
    EXPECT_NEAR(second.rtt().count(), 50.0F, 5.0F);
}

TEST(Connection, OnlyTheFirstAckOfADatagramTakesAnRttSample) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection first{kToken, settings, clock.now()};
    Connection second{kToken, settings, clock.now()};
    ASSERT_TRUE(deliver(second, sendOne(first, clock.now()), clock.now()));
    clock.advance(std::chrono::milliseconds{20});
    const network::Bytes reply = sendOne(second, clock.now());

    ASSERT_TRUE(deliver(first, reply, clock.now()));
    // One sample of 20 ms from 100 ms: 100 + (20 - 100) * 0.1.
    EXPECT_NEAR(first.rtt().count(), 92.0F, 0.01F);
    clock.advance(std::chrono::milliseconds{500});
    ASSERT_TRUE(deliver(first, reply, clock.now()));

    EXPECT_NEAR(first.rtt().count(), 92.0F, 0.01F);
}

TEST(Connection, ReceivingUpdatesTheLastReceiveTime) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};
    const network::TimePoint created = clock.now();
    EXPECT_EQ(receiver.lastReceiveTime(), created);

    clock.advance(std::chrono::milliseconds{250});
    ASSERT_TRUE(deliver(receiver, sendOne(sender, clock.now()), clock.now()));

    EXPECT_EQ(receiver.lastReceiveTime(), clock.now());
}

TEST(Connection, AKeepaliveCarriesAcksAndNoMessage) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};
    ASSERT_TRUE(deliver(receiver, sendOne(sender, clock.now()), clock.now()));
    ASSERT_EQ(receiver.takeMessages().size(), 1U);

    clock.advance(std::chrono::milliseconds{100});
    const auto packets = receiver.buildPackets(clock.now());
    ASSERT_EQ(packets.size(), 1U);
    ASSERT_TRUE(deliver(sender, packets.front(), clock.now()));

    EXPECT_TRUE(sender.takeMessages().empty());
    EXPECT_EQ(headerOf(packets.front())->ack, 0);
}

TEST(Connection, AMalformedMessageRejectsTheWholeDatagramAndChangesNothing) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection connection{kToken, settings, clock.now()};
    const std::vector<std::byte> valid = toBytes({0, 2, 0, 0xAA, 0xBB});
    const std::vector<std::vector<int>> malformed = {
        {2, 1, 0, 0xCC},     // Unknown channel.
        {0, 0, 0},           // Length 0.
        {0, 0xE9, 0x03, 1},  // Length 1001.
        {0, 5, 0, 1, 2},     // Length past the end.
        {0, 1},              // Header cut short.
    };

    for (const auto& bytes : malformed) {
        clock.advance(std::chrono::milliseconds{10});
        std::vector<std::byte> messages = valid;
        for (const std::byte byte : toBytes(bytes)) {
            messages.push_back(byte);
        }
        EXPECT_FALSE(receiveRaw(connection, messages, clock.now()));
        EXPECT_FALSE(receiveRaw(connection, toBytes(bytes), clock.now()));
        EXPECT_TRUE(connection.takeMessages().empty());
        EXPECT_EQ(connection.lastReceiveTime(), network::TimePoint{});
        EXPECT_FLOAT_EQ(connection.rtt().count(), 100.0F);
    }
    EXPECT_EQ(currentHeader(connection, clock).ack, kNothingAcked);
}

TEST(Connection, AValidRawDatagramIsAccepted) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection connection{kToken, settings, clock.now()};

    ASSERT_TRUE(receiveRaw(connection, toBytes({1, 2, 0, 0xAA, 0xBB, 0, 1, 0, 0xCC}), clock.now()));

    const auto messages = connection.takeMessages();
    ASSERT_EQ(messages.size(), 2U);
    EXPECT_EQ(messages[0].channel, network::Channel::kReliable);
    EXPECT_EQ(messages[0].payload, toBytes({0xAA, 0xBB}));
    EXPECT_EQ(messages[1].channel, network::Channel::kUnreliable);
    EXPECT_EQ(messages[1].payload, toBytes({0xCC}));
}
