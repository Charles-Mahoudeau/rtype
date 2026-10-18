/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RandomSourceTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <set>

#include "rtype/network/RandomSource.hpp"

TEST(RandomSource, SystemRandomIsCallable) {
    const rtype::network::RandomSource random = rtype::network::makeSystemRandom();

    EXPECT_TRUE(static_cast<bool>(random));
}

TEST(RandomSource, SystemRandomReturnsDifferentValues) {
    const rtype::network::RandomSource random = rtype::network::makeSystemRandom();

    EXPECT_NE(random(), random());
}

TEST(RandomSource, SystemRandomFillsTheHighBits) {
    const rtype::network::RandomSource random = rtype::network::makeSystemRandom();
    std::set<std::uint64_t> highWords;

    for (int i = 0; i < 16; ++i) {
        highWords.insert(random() >> 32U);
    }
    EXPECT_GT(highWords.size(), 1U);
}

TEST(RandomSource, CopiesShareTheSameDevice) {
    const rtype::network::RandomSource random = rtype::network::makeSystemRandom();
    const rtype::network::RandomSource copy = random;

    EXPECT_NE(random(), copy());
}

TEST(RandomSource, AFixedSequenceCanBeInjected) {
    std::uint64_t next = 0;
    const rtype::network::RandomSource fixed = [&next]() { return ++next; };

    EXPECT_EQ(fixed(), 1U);
    EXPECT_EQ(fixed(), 2U);
}
