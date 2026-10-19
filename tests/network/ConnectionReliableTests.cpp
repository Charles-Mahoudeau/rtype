/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConnectionReliableTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
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
constexpr std::chrono::milliseconds kTick{10};

network::Bytes encode(const std::uint32_t value) {
    network::Bytes bytes;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        bytes.push_back(static_cast<std::byte>((value >> shift) & 0xFFU));
    }
    return bytes;
}

std::uint32_t decode(const network::Bytes& bytes) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < 4 && i < bytes.size(); ++i) {
        value |= static_cast<std::uint32_t>(bytes[i]) << (8U * i);
    }
    return value;
}

std::size_t messageBytesIn(const std::vector<network::Bytes>& datagrams) {
    std::size_t total = 0;
    for (const auto& datagram : datagrams) {
        const auto decoded = protocol::PacketCodec::decode(datagram);
        if (decoded && std::holds_alternative<protocol::Data>(*decoded)) {
            total += std::get<protocol::Data>(*decoded).messages.size();
        }
    }
    return total;
}

void deliver(Connection& to, const network::Bytes& datagram, const network::TimePoint now) {
    const auto decoded = protocol::PacketCodec::decode(datagram);
    if (decoded && std::holds_alternative<protocol::Data>(*decoded)) {
        to.receive(std::get<protocol::Data>(*decoded), now);
    }
}

/// @brief Sends `count` numbered reliable messages from endpoint 0 to endpoint 1 through a lossy link, `perTick` per
/// tick, and checks that they come out once and in order.
void runTransfer(const std::uint32_t count, const std::uint32_t perTick, const tests::LinkConditions& conditions,
                 const std::uint32_t seed) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    tests::SimulatedLink link{conditions, seed};
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};

    std::uint32_t next = 0;
    std::uint32_t expected = 0;
    for (std::uint32_t tick = 0; tick < 200000 && expected < count; ++tick) {
        for (std::uint32_t i = 0; i < perTick && next < count; ++i) {
            ASSERT_TRUE(sender.send(network::Channel::kReliable, encode(next++)));
        }
        for (auto& datagram : sender.buildPackets(clock.now())) {
            link.send(1, std::move(datagram), clock.now());
        }
        for (auto& datagram : receiver.buildPackets(clock.now())) {
            link.send(0, std::move(datagram), clock.now());
        }
        clock.advance(kTick);
        for (const auto& datagram : link.receive(1, clock.now())) {
            deliver(receiver, datagram, clock.now());
        }
        for (const auto& datagram : link.receive(0, clock.now())) {
            deliver(sender, datagram, clock.now());
        }
        for (const auto& message : receiver.takeMessages()) {
            ASSERT_EQ(message.channel, network::Channel::kReliable);
            ASSERT_EQ(decode(message.payload), expected) << "seed " << seed;
            ++expected;
        }
        ASSERT_FALSE(sender.isOverflowed());
    }
    EXPECT_EQ(expected, count) << "seed " << seed;
    EXPECT_EQ(receiver.protocolErrorCount(), 0U);
}
}  // namespace

TEST(ConnectionReliable, ReliableMessagesGoBeforeUnreliableOnes) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};

    ASSERT_TRUE(sender.send(network::Channel::kUnreliable, encode(1)));
    ASSERT_TRUE(sender.send(network::Channel::kReliable, encode(2)));
    const auto packets = sender.buildPackets(clock.now());

    ASSERT_EQ(packets.size(), 1U);
    deliver(receiver, packets.front(), clock.now());
    EXPECT_EQ(receiver.takeMessages().size(), 2U);
    // The reliable message is written first: 5 header bytes, then the unreliable one with 3.
    const auto decoded = protocol::PacketCodec::decode(packets.front());
    ASSERT_TRUE(decoded);
    const auto& data = std::get<protocol::Data>(*decoded);
    ASSERT_EQ(data.messages.size(), 5U + 4U + 3U + 4U);
    EXPECT_EQ(data.messages[0], std::byte{1});
}

TEST(ConnectionReliable, AMessageIsResentOnlyAfterTheIntervalAndNeverOnceAcked) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};

    ASSERT_TRUE(sender.send(network::Channel::kReliable, encode(1)));
    ASSERT_EQ(sender.buildPackets(clock.now()).size(), 1U);  // Lost.

    // The resend delay is 1.25 x the initial RTT of 100 ms.
    clock.advance(std::chrono::milliseconds{124});
    EXPECT_EQ(messageBytesIn(sender.buildPackets(clock.now())), 0U);  // At most a keepalive.
    clock.advance(std::chrono::milliseconds{1});
    auto packets = sender.buildPackets(clock.now());
    ASSERT_EQ(packets.size(), 1U);

    deliver(receiver, packets.front(), clock.now());
    ASSERT_EQ(receiver.takeMessages().size(), 1U);
    clock.advance(std::chrono::milliseconds{100});
    for (const auto& datagram : receiver.buildPackets(clock.now())) {
        deliver(sender, datagram, clock.now());
    }

    clock.advance(std::chrono::seconds{1});
    for (const auto& datagram : sender.buildPackets(clock.now())) {
        deliver(receiver, datagram, clock.now());
    }
    EXPECT_TRUE(receiver.takeMessages().empty());
}

TEST(ConnectionReliable, ADuplicatedDatagramDeliversTheMessageOnce) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};

    ASSERT_TRUE(sender.send(network::Channel::kReliable, encode(1)));
    const auto packets = sender.buildPackets(clock.now());
    ASSERT_EQ(packets.size(), 1U);
    deliver(receiver, packets.front(), clock.now());
    deliver(receiver, packets.front(), clock.now());

    EXPECT_EQ(receiver.takeMessages().size(), 1U);
}

TEST(ConnectionReliable, TheWindowHoldsBackMessage257UntilTheFirstIsAcked) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};
    for (std::uint32_t i = 0; i < 257; ++i) {
        ASSERT_TRUE(sender.send(network::Channel::kReliable, encode(i)));
    }

    std::vector<network::Bytes> first = sender.buildPackets(clock.now());
    for (const auto& datagram : first) {
        deliver(receiver, datagram, clock.now());
    }
    EXPECT_EQ(receiver.takeMessages().size(), 256U);

    // Everything is acked, so the last message goes out.
    clock.advance(std::chrono::milliseconds{100});
    for (const auto& datagram : receiver.buildPackets(clock.now())) {
        deliver(sender, datagram, clock.now());
    }
    for (const auto& datagram : sender.buildPackets(clock.now())) {
        deliver(receiver, datagram, clock.now());
    }
    const auto last = receiver.takeMessages();
    ASSERT_EQ(last.size(), 1U);
    EXPECT_EQ(decode(last[0].payload), 256U);
}

TEST(ConnectionReliable, ABurstOf64MessagesSpreadsOverSeveralDatagramsInOrder) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection sender{kToken, settings, clock.now()};
    Connection receiver{kToken, settings, clock.now()};
    for (std::uint32_t i = 0; i < 64; ++i) {
        network::Bytes payload;
        const network::Bytes index = encode(i);
        payload.assign(index.begin(), index.end());
        payload.resize(100);
        ASSERT_TRUE(sender.send(network::Channel::kReliable, payload));
    }

    const auto packets = sender.buildPackets(clock.now());
    EXPECT_GT(packets.size(), 1U);
    EXPECT_LE(packets.size(), protocol::kMaxDatagramsPerFlush);
    for (const auto& datagram : packets) {
        EXPECT_LE(datagram.size(), protocol::kMaxPacketSize);
        deliver(receiver, datagram, clock.now());
    }
    const auto messages = receiver.takeMessages();
    ASSERT_EQ(messages.size(), 64U);
    for (std::uint32_t i = 0; i < 64; ++i) {
        EXPECT_EQ(decode(messages[i].payload), i);
    }
}

TEST(ConnectionReliable, OverflowsAtThe1025thPendingMessageAndStaysSo) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection connection{kToken, settings, clock.now()};

    for (std::size_t i = 0; i < protocol::kMaxPendingReliable; ++i) {
        ASSERT_TRUE(connection.send(network::Channel::kReliable, encode(0)));
    }
    EXPECT_FALSE(connection.isOverflowed());
    EXPECT_TRUE(connection.send(network::Channel::kReliable, encode(0)));
    EXPECT_TRUE(connection.isOverflowed());
    EXPECT_FALSE(connection.send(network::Channel::kReliable, encode(0)));
    EXPECT_TRUE(connection.isOverflowed());
}

TEST(ConnectionReliable, AnIdFarBeyondTheWindowIsCountedAndTheRestOfTheDatagramKept) {
    const network::NetworkSettings settings;
    tests::FakeClock clock;
    Connection connection{kToken, settings, clock.now()};
    // Reliable id 1000 (0x03E8), then an unreliable message.
    const std::vector<std::byte> messages = {std::byte{1}, std::byte{0xE8}, std::byte{0x03}, std::byte{1},
                                             std::byte{0}, std::byte{0xAA}, std::byte{0},    std::byte{1},
                                             std::byte{0}, std::byte{0xBB}};
    const protocol::Data data{.sessionToken = kToken, .sequence = 0, .ack = 0, .ackBits = 0, .messages = messages};

    EXPECT_TRUE(connection.receive(data, clock.now()));

    EXPECT_EQ(connection.protocolErrorCount(), 1U);
    const auto received = connection.takeMessages();
    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(received[0].channel, network::Channel::kUnreliable);
}

TEST(ConnectionReliable, 5000MessagesArriveOnceAndInOrderUnder30PercentLoss) {
    for (std::uint32_t seed = 1; seed <= 20; ++seed) {
        runTransfer(5000, 5,
                    tests::LinkConditions{.loss = 0.3,
                                          .duplicate = 0.1,
                                          .reorder = 0.1,
                                          .delay = std::chrono::milliseconds{20},
                                          .jitter = std::chrono::milliseconds{10}},
                    seed);
    }
}

TEST(ConnectionReliable, 70000MessagesCrossTheIdWrap) {
    runTransfer(70000, 8,
                tests::LinkConditions{.loss = 0.1,
                                      .duplicate = 0.05,
                                      .reorder = 0.05,
                                      .delay = std::chrono::milliseconds{20},
                                      .jitter = std::chrono::milliseconds{10}},
                7);
}
