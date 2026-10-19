/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SequenceTests
*/

#include <gtest/gtest.h>

#include "rtype/network/protocol/Sequence.hpp"

namespace protocol = rtype::network::protocol;

static_assert(protocol::isNewer(1, 0));
static_assert(!protocol::isNewer(0, 1));
static_assert(protocol::isNewer(3, 65530));
static_assert(!protocol::isNewer(65530, 3));
static_assert(protocol::isNewer(0, 65535));
static_assert(!protocol::isNewer(65535, 0));
static_assert(!protocol::isNewer(0, 0));
static_assert(!protocol::isNewer(65535, 65535));
static_assert(protocol::isNewer(32767, 0));
static_assert(!protocol::isNewer(0, 32767));
static_assert(!protocol::isNewer(32768, 0));
static_assert(!protocol::isNewer(0, 32768));
static_assert(!protocol::isNewer(10, 32778));
static_assert(!protocol::isNewer(32778, 10));
static_assert(noexcept(protocol::isNewer(1, 0)));

TEST(Sequence, OneStepAhead) {
    EXPECT_TRUE(protocol::isNewer(1, 0));
    EXPECT_FALSE(protocol::isNewer(0, 1));
}

TEST(Sequence, AcrossTheWrap) {
    EXPECT_TRUE(protocol::isNewer(3, 65530));
    EXPECT_FALSE(protocol::isNewer(65530, 3));
    EXPECT_TRUE(protocol::isNewer(0, 65535));
    EXPECT_FALSE(protocol::isNewer(65535, 0));
}

TEST(Sequence, EqualValuesAreNotNewer) {
    EXPECT_FALSE(protocol::isNewer(0, 0));
    EXPECT_FALSE(protocol::isNewer(1234, 1234));
    EXPECT_FALSE(protocol::isNewer(65535, 65535));
}

TEST(Sequence, HalfRange) {
    EXPECT_TRUE(protocol::isNewer(32767, 0));
    EXPECT_FALSE(protocol::isNewer(0, 32767));
    EXPECT_FALSE(protocol::isNewer(32768, 0));
    EXPECT_FALSE(protocol::isNewer(0, 32768));
}

TEST(Sequence, ExactlyOneDirectionIsNewer) {
    for (std::uint32_t b = 0; b < 65536; b += 251) {
        for (std::uint32_t step = 1; step < 65536; step += 997) {
            const auto a = static_cast<std::uint16_t>(b + step);
            const bool forward = protocol::isNewer(a, static_cast<std::uint16_t>(b));
            const bool backward = protocol::isNewer(static_cast<std::uint16_t>(b), a);
            if (step == 32768) {
                EXPECT_FALSE(forward);
                EXPECT_FALSE(backward);
            } else {
                EXPECT_NE(forward, backward) << "b=" << b << " step=" << step;
                EXPECT_EQ(forward, step < 32768) << "b=" << b << " step=" << step;
            }
        }
    }
}
