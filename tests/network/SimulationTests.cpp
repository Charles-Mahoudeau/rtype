/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SimulationTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "rtype/network/RandomSource.hpp"
#include "rtype/network/Types.hpp"
#include "rtype/network/simulation/LinkSimulator.hpp"

namespace network = rtype::network;
namespace simulation = rtype::network::simulation;
using std::chrono::milliseconds;

namespace {
/// A random source that gives @p values in turn, forever.
network::RandomSource cycle(std::vector<std::uint64_t> values) {
    return [values = std::move(values), next = std::size_t{0}]() mutable { return values[next++ % values.size()]; };
}

/// @return A datagram whose first byte is @p tag.
simulation::SimulatedDatagram tagged(const std::uint8_t tag) {
    return simulation::SimulatedDatagram{.peer = {}, .bytes = {static_cast<std::byte>(tag)}};
}

std::uint8_t tagOf(const simulation::SimulatedDatagram& datagram) {
    return static_cast<std::uint8_t>(datagram.bytes[0]);
}
}  // namespace

TEST(LinkSimulator, IsOffAndIdleByDefaultAndPassesDatagramsInOrder) {
    simulation::LinkSimulator link{cycle({0})};
    const network::TimePoint now = network::Clock::now();
    EXPECT_TRUE(link.isIdle());
    EXPECT_FALSE(link.settings().isActive());
    link.submit(tagged(1), now);
    link.submit(tagged(2), now);
    const auto ready = link.takeReady(now);
    ASSERT_EQ(ready.size(), 2U);
    EXPECT_EQ(tagOf(ready[0]), 1U);
    EXPECT_EQ(tagOf(ready[1]), 2U);
    EXPECT_EQ(link.droppedCount(), 0U);
    EXPECT_TRUE(link.isIdle());
}

TEST(LinkSimulator, DropsTheConfiguredShareWithAFixedSource) {
    // 0 is always under the threshold (dropped), 9999 never (kept): half the datagrams go.
    simulation::LinkSimulator link{cycle({0, 9999})};
    link.configure({.latencyMs = 0, .jitterMs = 0, .lossPercent = 50.0F});
    const network::TimePoint now = network::Clock::now();
    for (int i = 0; i < 100; ++i) {
        link.submit(tagged(static_cast<std::uint8_t>(i)), now);
    }
    EXPECT_EQ(link.droppedCount(), 50U);
    EXPECT_EQ(link.takeReady(now).size(), 50U);
}

TEST(LinkSimulator, TotalLossDropsEverythingAndNoLossDropsNothing) {
    simulation::LinkSimulator link{cycle({9999})};
    const network::TimePoint now = network::Clock::now();
    link.configure({.lossPercent = 100.0F});
    link.submit(tagged(1), now);
    EXPECT_EQ(link.droppedCount(), 1U);
    EXPECT_TRUE(link.takeReady(now).empty());
    link.configure({.lossPercent = 0.0F});
    link.submit(tagged(2), now);
    EXPECT_EQ(link.droppedCount(), 1U);
    EXPECT_EQ(link.takeReady(now).size(), 1U);
}

TEST(LinkSimulator, HoldsDatagramsForTheLatencyAndKeepsTheirOrder) {
    simulation::LinkSimulator link{cycle({0})};
    link.configure({.latencyMs = 100});
    const network::TimePoint start = network::Clock::now();
    link.submit(tagged(1), start);
    link.submit(tagged(2), start);
    link.submit(tagged(3), start + milliseconds{10});
    EXPECT_FALSE(link.isIdle());
    EXPECT_EQ(link.pendingCount(), 3U);
    EXPECT_TRUE(link.takeReady(start + milliseconds{99}).empty());
    const auto first = link.takeReady(start + milliseconds{100});
    ASSERT_EQ(first.size(), 2U);
    EXPECT_EQ(tagOf(first[0]), 1U);
    EXPECT_EQ(tagOf(first[1]), 2U);
    const auto second = link.takeReady(start + milliseconds{110});
    ASSERT_EQ(second.size(), 1U);
    EXPECT_EQ(tagOf(second[0]), 3U);
}

TEST(LinkSimulator, JitterAddsARandomDelayAndCanReorder) {
    // Jitter 50: the delay is the draw modulo 51. The first datagram draws 40, the second 10.
    simulation::LinkSimulator link{cycle({40, 10})};
    link.configure({.latencyMs = 100, .jitterMs = 50});
    const network::TimePoint start = network::Clock::now();
    link.submit(tagged(1), start);
    link.submit(tagged(2), start);
    EXPECT_TRUE(link.takeReady(start + milliseconds{109}).empty());
    const auto early = link.takeReady(start + milliseconds{110});
    ASSERT_EQ(early.size(), 1U);
    EXPECT_EQ(tagOf(early[0]), 2U);
    const auto late = link.takeReady(start + milliseconds{140});
    ASSERT_EQ(late.size(), 1U);
    EXPECT_EQ(tagOf(late[0]), 1U);
}

TEST(LinkSimulator, TurningItOffStillReleasesWhatIsHeld) {
    simulation::LinkSimulator link{cycle({0})};
    link.configure({.latencyMs = 100});
    const network::TimePoint start = network::Clock::now();
    link.submit(tagged(1), start);
    link.configure({});
    EXPECT_FALSE(link.isIdle());  // one datagram is still held
    EXPECT_EQ(link.takeReady(start + milliseconds{100}).size(), 1U);
    EXPECT_TRUE(link.isIdle());
}

TEST(LinkSimulator, TakeAllGivesEverythingInDueOrder) {
    simulation::LinkSimulator link{cycle({0})};
    link.configure({.latencyMs = 1000});
    const network::TimePoint start = network::Clock::now();
    link.submit(tagged(2), start + milliseconds{5});
    link.submit(tagged(1), start);
    const auto all = link.takeAll();
    ASSERT_EQ(all.size(), 2U);
    EXPECT_EQ(tagOf(all[0]), 1U);
    EXPECT_EQ(tagOf(all[1]), 2U);
    EXPECT_EQ(link.pendingCount(), 0U);
}

TEST(LinkSimulator, ClampsBadValues) {
    simulation::LinkSimulator link{cycle({0})};
    auto applied = link.configure({.latencyMs = 60000, .jitterMs = 65535, .lossPercent = 150.0F});
    EXPECT_EQ(applied.latencyMs, simulation::kMaxDelayMs);
    EXPECT_EQ(applied.jitterMs, simulation::kMaxDelayMs);
    EXPECT_FLOAT_EQ(applied.lossPercent, 100.0F);
    EXPECT_EQ(link.settings(), applied);
    applied = link.configure({.lossPercent = -3.0F});
    EXPECT_FLOAT_EQ(applied.lossPercent, 0.0F);
    applied = link.configure({.lossPercent = std::numeric_limits<float>::quiet_NaN()});
    EXPECT_FLOAT_EQ(applied.lossPercent, 0.0F);
    applied = link.configure({.lossPercent = std::numeric_limits<float>::infinity()});
    EXPECT_FLOAT_EQ(applied.lossPercent, 100.0F);
    EXPECT_FALSE(simulation::SimulationSettings{}.isActive());
}
