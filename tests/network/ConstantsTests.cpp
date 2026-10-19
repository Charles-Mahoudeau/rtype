/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ConstantsTests
*/

#include <gtest/gtest.h>

#include "rtype/network/protocol/Constants.hpp"

namespace protocol = rtype::network::protocol;

TEST(Constants, ProtocolValues) {
    EXPECT_EQ(protocol::kProtocolId, 0x52545950U);
    EXPECT_EQ(protocol::kProtocolVersion, 1U);
    EXPECT_EQ(protocol::kMaxPacketSize, 1200U);
    EXPECT_EQ(protocol::kMaxMessageSize, 1000U);
    EXPECT_EQ(protocol::kHandshakePadding, 64U);
    EXPECT_EQ(protocol::kReliableWindow, 256U);
    EXPECT_EQ(protocol::kMaxPendingReliable, 1024U);
    EXPECT_EQ(protocol::kMaxDatagramsPerFlush, 8U);
    EXPECT_EQ(protocol::kSentPacketHistory, 1024U);
}

TEST(Constants, MessageFitsInPacket) { EXPECT_LT(protocol::kMaxMessageSize, protocol::kMaxPacketSize); }
