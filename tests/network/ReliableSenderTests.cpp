/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReliableSenderTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "FakeClock.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/connection/ReliableSender.hpp"
#include "rtype/network/protocol/Constants.hpp"

namespace network = rtype::network;
namespace protocol = rtype::network::protocol;
namespace tests = rtype::network::tests;
using rtype::network::connection::ReliableSender;

namespace {
constexpr std::chrono::milliseconds kInterval{50};

network::Bytes payload() { return network::Bytes(4, std::byte{7}); }
}  // namespace

TEST(ReliableSender, IdsStartAtZeroAndGrowByOne) {
    ReliableSender sender;
    EXPECT_EQ(sender.push(payload()), 0);
    EXPECT_EQ(sender.push(payload()), 1);
    EXPECT_EQ(sender.push(payload()), 2);
    EXPECT_EQ(sender.pendingCount(), 3U);
}

TEST(ReliableSender, IdsWrapAfter65535) {
    ReliableSender sender;
    for (std::uint32_t i = 0; i < 65536; ++i) {
        EXPECT_EQ(sender.push(payload()), static_cast<std::uint16_t>(i));
        sender.acknowledge(static_cast<std::uint16_t>(i));
    }
    EXPECT_EQ(sender.push(payload()), 0);
}

TEST(ReliableSender, ANewMessageIsDueAndOldestComesFirst) {
    ReliableSender sender;
    tests::FakeClock clock;
    sender.push(payload());
    sender.push(payload());

    const auto due = sender.collectDue(clock.now(), kInterval);
    ASSERT_EQ(due.size(), 2U);
    EXPECT_EQ(due[0]->id, 0);
    EXPECT_EQ(due[1]->id, 1);
    EXPECT_FALSE(due[0]->sent);
}

TEST(ReliableSender, AMessageIsNotDueBeforeTheIntervalAndIsDueAfter) {
    ReliableSender sender;
    tests::FakeClock clock;
    sender.push(payload());
    sender.markSent(0, clock.now());

    clock.advance(kInterval - std::chrono::milliseconds{1});
    EXPECT_TRUE(sender.collectDue(clock.now(), kInterval).empty());
    clock.advance(std::chrono::milliseconds{1});
    const auto due = sender.collectDue(clock.now(), kInterval);
    ASSERT_EQ(due.size(), 1U);
    EXPECT_TRUE(due[0]->sent);
}

TEST(ReliableSender, AnAcknowledgedMessageIsNeverDueAgain) {
    ReliableSender sender;
    tests::FakeClock clock;
    sender.push(payload());
    sender.push(payload());
    sender.markSent(0, clock.now());
    sender.acknowledge(0);
    sender.acknowledge(0);

    clock.advance(std::chrono::seconds{1});
    const auto due = sender.collectDue(clock.now(), kInterval);
    ASSERT_EQ(due.size(), 1U);
    EXPECT_EQ(due[0]->id, 1);
    EXPECT_EQ(sender.pendingCount(), 1U);
}

TEST(ReliableSender, UnknownIdsAreIgnored) {
    ReliableSender sender;
    sender.push(payload());
    sender.acknowledge(5);
    sender.markSent(5, network::TimePoint{});
    sender.acknowledge(0xFFFF);
    EXPECT_EQ(sender.pendingCount(), 1U);
}

TEST(ReliableSender, TheWindowHoldsBackMessage256UntilTheOldestIsAcked) {
    ReliableSender sender;
    tests::FakeClock clock;
    for (std::size_t i = 0; i < protocol::kReliableWindow + 1; ++i) {
        sender.push(payload());
    }

    auto due = sender.collectDue(clock.now(), kInterval);
    ASSERT_EQ(due.size(), protocol::kReliableWindow);
    EXPECT_EQ(due.back()->id, protocol::kReliableWindow - 1);

    sender.acknowledge(1);
    EXPECT_EQ(sender.collectDue(clock.now(), kInterval).size(), protocol::kReliableWindow - 1);

    sender.acknowledge(0);
    due = sender.collectDue(clock.now(), kInterval);
    EXPECT_EQ(due.size(), protocol::kReliableWindow - 1);
    EXPECT_EQ(due.back()->id, protocol::kReliableWindow);
}

TEST(ReliableSender, OverflowsWhenMoreThan1024ArePending) {
    ReliableSender sender;
    for (std::size_t i = 0; i < protocol::kMaxPendingReliable; ++i) {
        sender.push(payload());
    }
    EXPECT_FALSE(sender.isOverflowed());

    sender.push(payload());
    EXPECT_TRUE(sender.isOverflowed());
    sender.acknowledge(0);
    EXPECT_TRUE(sender.isOverflowed());

    sender.push(payload());
    EXPECT_EQ(sender.pendingCount(), protocol::kMaxPendingReliable);
}
