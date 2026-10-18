/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SimulatedLinkTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "FakeClock.hpp"
#include "SimulatedLink.hpp"
#include "rtype/network/Types.hpp"

namespace {
using namespace std::chrono_literals;
using rtype::network::Bytes;
using rtype::network::tests::FakeClock;
using rtype::network::tests::LinkConditions;
using rtype::network::tests::SimulatedLink;

Bytes makeDatagram(const std::uint32_t id) {
    return Bytes{static_cast<std::byte>(id & 0xFFU), static_cast<std::byte>((id >> 8U) & 0xFFU)};
}

std::uint32_t idOf(const Bytes& datagram) {
    return static_cast<std::uint32_t>(datagram[0]) | (static_cast<std::uint32_t>(datagram[1]) << 8U);
}

/// Sends `count` datagrams 1 ms apart to endpoint 1, then collects everything until all is delivered.
std::vector<std::uint32_t> run(SimulatedLink& link, const std::uint32_t count) {
    FakeClock clock;
    std::vector<std::uint32_t> received;

    for (std::uint32_t i = 0; i < count; ++i) {
        link.send(1, makeDatagram(i), clock.now());
        clock.advance(1ms);
    }
    clock.advance(10s);
    for (const Bytes& datagram : link.receive(1, clock.now())) {
        received.push_back(idOf(datagram));
    }
    return received;
}
}  // namespace

TEST(FakeClock, StartsStillAndOnlyMovesWhenAdvanced) {
    FakeClock clock;
    const auto start = clock.now();

    EXPECT_EQ(clock.now(), start);
    clock.advance(2s);
    EXPECT_EQ(clock.now() - start, 2s);
    clock.advance(500us);
    EXPECT_EQ(clock.now() - start, 2s + 500us);
}

TEST(SimulatedLink, PerfectConditionsDeliverEverythingInOrderAfterTheDelay) {
    FakeClock clock;
    SimulatedLink link{{.delay = 50ms}, 1};

    for (std::uint32_t i = 0; i < 5; ++i) {
        link.send(1, makeDatagram(i), clock.now());
    }
    clock.advance(49ms);
    EXPECT_TRUE(link.receive(1, clock.now()).empty());
    clock.advance(1ms);
    const std::vector<Bytes> received = link.receive(1, clock.now());
    ASSERT_EQ(received.size(), 5U);
    for (std::uint32_t i = 0; i < 5; ++i) {
        EXPECT_EQ(idOf(received[i]), i);
    }
}

TEST(SimulatedLink, ReceiveForgetsWhatItReturned) {
    FakeClock clock;
    SimulatedLink link{{}, 1};

    link.send(1, makeDatagram(7), clock.now());
    EXPECT_EQ(link.receive(1, clock.now()).size(), 1U);
    EXPECT_TRUE(link.receive(1, clock.now()).empty());
}

TEST(SimulatedLink, DatagramsAreOnlyGivenToTheirEndpoint) {
    FakeClock clock;
    SimulatedLink link{{}, 1};

    link.send(0, makeDatagram(1), clock.now());
    link.send(1, makeDatagram(2), clock.now());
    const std::vector<Bytes> atZero = link.receive(0, clock.now());
    const std::vector<Bytes> atOne = link.receive(1, clock.now());
    ASSERT_EQ(atZero.size(), 1U);
    ASSERT_EQ(atOne.size(), 1U);
    EXPECT_EQ(idOf(atZero[0]), 1U);
    EXPECT_EQ(idOf(atOne[0]), 2U);
}

TEST(SimulatedLink, TotalLossDeliversNothing) {
    SimulatedLink link{{.loss = 1.0}, 1};

    EXPECT_TRUE(run(link, 100).empty());
    EXPECT_EQ(link.stats().sent, 100U);
    EXPECT_EQ(link.stats().dropped, 100U);
    EXPECT_EQ(link.stats().delivered, 0U);
}

TEST(SimulatedLink, ObservedLossIsCloseToTheSetting) {
    constexpr std::uint32_t kCount = 10'000;
    SimulatedLink link{{.loss = 0.3}, 42};

    const std::vector<std::uint32_t> received = run(link, kCount);
    const double observed = 1.0 - (static_cast<double>(received.size()) / kCount);
    EXPECT_NEAR(observed, 0.3, 0.02);
    EXPECT_EQ(link.stats().dropped + received.size(), kCount);
}

TEST(SimulatedLink, ObservedDuplicationIsCloseToTheSetting) {
    constexpr std::uint32_t kCount = 10'000;
    SimulatedLink link{{.duplicate = 0.2}, 42};

    const std::vector<std::uint32_t> received = run(link, kCount);
    EXPECT_NEAR(static_cast<double>(link.stats().duplicated) / kCount, 0.2, 0.02);
    EXPECT_EQ(received.size(), kCount + link.stats().duplicated);
}

TEST(SimulatedLink, ReorderingMakesLaterDatagramsOvertake) {
    constexpr std::uint32_t kCount = 10'000;
    SimulatedLink link{{.reorder = 0.2, .delay = 5ms}, 42};

    const std::vector<std::uint32_t> received = run(link, kCount);
    ASSERT_EQ(received.size(), kCount);
    EXPECT_NEAR(static_cast<double>(link.stats().reordered) / kCount, 0.2, 0.02);
    std::size_t inversions = 0;
    for (std::size_t i = 1; i < received.size(); ++i) {
        inversions += received[i] < received[i - 1] ? 1U : 0U;
    }
    EXPECT_GT(inversions, 0U);
}

TEST(SimulatedLink, WithoutReorderingTheOrderIsKept) {
    SimulatedLink link{{.delay = 5ms}, 42};

    const std::vector<std::uint32_t> received = run(link, 1000);
    ASSERT_EQ(received.size(), 1000U);
    for (std::uint32_t i = 0; i < 1000; ++i) {
        EXPECT_EQ(received[i], i);
    }
}

TEST(SimulatedLink, JitterStaysWithinItsBounds) {
    FakeClock clock;
    SimulatedLink link{{.delay = 50ms, .jitter = 10ms}, 3};

    for (std::uint32_t i = 0; i < 200; ++i) {
        link.send(1, makeDatagram(i), clock.now());
    }
    clock.advance(49ms);
    EXPECT_TRUE(link.receive(1, clock.now()).empty());
    clock.advance(11ms);
    EXPECT_EQ(link.receive(1, clock.now()).size(), 200U);
}

TEST(SimulatedLink, SameSeedGivesIdenticalResults) {
    const LinkConditions conditions{.loss = 0.2, .duplicate = 0.1, .reorder = 0.1, .delay = 20ms, .jitter = 10ms};
    SimulatedLink first{conditions, 99};
    SimulatedLink second{conditions, 99};

    EXPECT_EQ(run(first, 2000), run(second, 2000));
    EXPECT_EQ(first.stats().dropped, second.stats().dropped);
}

TEST(SimulatedLink, DifferentSeedsGiveDifferentResults) {
    const LinkConditions conditions{.loss = 0.3};
    SimulatedLink first{conditions, 1};
    SimulatedLink second{conditions, 2};

    EXPECT_NE(run(first, 2000), run(second, 2000));
}

TEST(SimulatedLink, ConditionsCanChangeAtAnyTime) {
    FakeClock clock;
    SimulatedLink link{{}, 1};

    link.setConditions({.loss = 1.0});
    link.send(1, makeDatagram(1), clock.now());
    link.setConditions({});
    link.send(1, makeDatagram(2), clock.now());
    const std::vector<Bytes> received = link.receive(1, clock.now());
    ASSERT_EQ(received.size(), 1U);
    EXPECT_EQ(idOf(received[0]), 2U);
}
