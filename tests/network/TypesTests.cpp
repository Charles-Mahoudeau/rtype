/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypesTests
*/

#include <gtest/gtest.h>

#include <type_traits>

#include "rtype/network/Types.hpp"

namespace {

using rtype::network::Channel;
using rtype::network::DisconnectReason;
using rtype::network::RejectReason;

}  // namespace

TEST(Types, NoClientIsZero) { EXPECT_EQ(rtype::network::kNoClient, 0U); }

TEST(Types, ClockIsMonotonic) { EXPECT_TRUE(rtype::network::Clock::is_steady); }

TEST(Types, ChannelWireValues) {
    EXPECT_EQ(static_cast<unsigned>(Channel::kUnreliable), 0U);
    EXPECT_EQ(static_cast<unsigned>(Channel::kReliable), 1U);
}

TEST(Types, DisconnectReasonWireValues) {
    EXPECT_EQ(static_cast<unsigned>(DisconnectReason::kGraceful), 1U);
    EXPECT_EQ(static_cast<unsigned>(DisconnectReason::kTimeout), 2U);
    EXPECT_EQ(static_cast<unsigned>(DisconnectReason::kKicked), 3U);
    EXPECT_EQ(static_cast<unsigned>(DisconnectReason::kProtocolError), 4U);
    EXPECT_EQ(static_cast<unsigned>(DisconnectReason::kManifestMismatch), 5U);
    EXPECT_EQ(static_cast<unsigned>(DisconnectReason::kOverflow), 6U);
    EXPECT_EQ(static_cast<unsigned>(DisconnectReason::kServerShutdown), 7U);
}

TEST(Types, RejectReasonWireValues) {
    EXPECT_EQ(static_cast<unsigned>(RejectReason::kVersionMismatch), 1U);
    EXPECT_EQ(static_cast<unsigned>(RejectReason::kServerFull), 2U);
    EXPECT_EQ(static_cast<unsigned>(RejectReason::kSessionExpired), 3U);
}
