/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ReliableReceiverTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "rtype/network/Types.hpp"
#include "rtype/network/connection/ReliableReceiver.hpp"
#include "rtype/network/protocol/Constants.hpp"

namespace network = rtype::network;
namespace protocol = rtype::network::protocol;
using rtype::network::connection::ReliableReceiver;

namespace {
network::Bytes payloadOf(const int value) { return network::Bytes(1, static_cast<std::byte>(value)); }
}  // namespace

TEST(ReliableReceiver, InOrderMessagesAreDeliveredAtOnce) {
    ReliableReceiver receiver;
    EXPECT_TRUE(receiver.receive(0, payloadOf(10)));
    EXPECT_TRUE(receiver.receive(1, payloadOf(11)));

    const auto delivered = receiver.takeDelivered();
    ASSERT_EQ(delivered.size(), 2U);
    EXPECT_EQ(delivered[0], payloadOf(10));
    EXPECT_EQ(delivered[1], payloadOf(11));
    EXPECT_TRUE(receiver.takeDelivered().empty());
}

TEST(ReliableReceiver, EarlyMessagesWaitForTheMissingOne) {
    ReliableReceiver receiver;
    EXPECT_TRUE(receiver.receive(2, payloadOf(12)));
    EXPECT_TRUE(receiver.receive(1, payloadOf(11)));
    EXPECT_TRUE(receiver.takeDelivered().empty());

    EXPECT_TRUE(receiver.receive(0, payloadOf(10)));
    const auto delivered = receiver.takeDelivered();
    ASSERT_EQ(delivered.size(), 3U);
    EXPECT_EQ(delivered[0], payloadOf(10));
    EXPECT_EQ(delivered[1], payloadOf(11));
    EXPECT_EQ(delivered[2], payloadOf(12));
}

TEST(ReliableReceiver, DuplicatesAreDroppedWithoutError) {
    ReliableReceiver receiver;
    EXPECT_TRUE(receiver.receive(0, payloadOf(10)));
    EXPECT_TRUE(receiver.receive(0, payloadOf(10)));
    EXPECT_TRUE(receiver.receive(3, payloadOf(13)));
    EXPECT_TRUE(receiver.receive(3, payloadOf(99)));
    EXPECT_TRUE(receiver.receive(1, payloadOf(11)));
    EXPECT_TRUE(receiver.receive(2, payloadOf(12)));
    EXPECT_TRUE(receiver.receive(1, payloadOf(11)));

    const auto delivered = receiver.takeDelivered();
    ASSERT_EQ(delivered.size(), 4U);
    EXPECT_EQ(delivered[3], payloadOf(13));
}

TEST(ReliableReceiver, TheLastIdOfTheWindowIsKeptAndTheNextOneIsAnError) {
    ReliableReceiver receiver;
    EXPECT_TRUE(receiver.receive(protocol::kReliableWindow - 1, payloadOf(1)));
    EXPECT_FALSE(receiver.receive(protocol::kReliableWindow, payloadOf(2)));
    EXPECT_FALSE(receiver.receive(0x4000, payloadOf(3)));
    EXPECT_TRUE(receiver.takeDelivered().empty());
}

TEST(ReliableReceiver, IdsKeepWorkingAcrossTheWrap) {
    ReliableReceiver receiver;
    for (std::uint32_t i = 0; i < 65536 + 10; ++i) {
        ASSERT_TRUE(receiver.receive(static_cast<std::uint16_t>(i), payloadOf(static_cast<int>(i & 0xFFU))));
    }
    EXPECT_EQ(receiver.takeDelivered().size(), 65536U + 10U);

    EXPECT_TRUE(receiver.receive(12, payloadOf(1)));
    EXPECT_TRUE(receiver.receive(10, payloadOf(2)));
    EXPECT_EQ(receiver.takeDelivered().size(), 1U);
}
