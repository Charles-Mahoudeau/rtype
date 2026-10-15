/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TickTests
*/

#include <gtest/gtest.h>

#include <limits>

#include "rtype/ecs/change/Tick.hpp"

namespace {
constexpr rtype::ecs::Tick kMaxTick = std::numeric_limits<rtype::ecs::Tick>::max();
}  // namespace

static_assert(rtype::ecs::isNewerThan(5, 3, 10), "usable at compile time");

TEST(Tick, ATickAfterTheLastRunIsNewer) {
    EXPECT_TRUE(rtype::ecs::isNewerThan(8, 5, 10));
    EXPECT_FALSE(rtype::ecs::isNewerThan(3, 5, 10));
}

TEST(Tick, ATickAtTheLastRunIsNotNewer) { EXPECT_FALSE(rtype::ecs::isNewerThan(5, 5, 10)); }

TEST(Tick, ATickAtThisRunIsNewerThanAnEarlierLastRun) { EXPECT_TRUE(rtype::ecs::isNewerThan(10, 9, 10)); }

TEST(Tick, ComparesAcrossTheWrap) {
    // The counter wrapped between the last run and now: thisRun is small, lastRun near the maximum.
    const rtype::ecs::Tick lastRun = kMaxTick - 5;
    const rtype::ecs::Tick thisRun = 10;
    EXPECT_TRUE(rtype::ecs::isNewerThan(3, lastRun, thisRun));              // after the wrap
    EXPECT_TRUE(rtype::ecs::isNewerThan(kMaxTick - 1, lastRun, thisRun));   // before the wrap, after lastRun
    EXPECT_FALSE(rtype::ecs::isNewerThan(kMaxTick - 9, lastRun, thisRun));  // before lastRun
}

TEST(Tick, ClampLeavesRecentTicksAlone) {
    rtype::ecs::Tick tick = 1000;
    rtype::ecs::clampTick(tick, 5000);
    EXPECT_EQ(tick, 1000U);
}

TEST(Tick, ClampMovesTooOldTicksToTheMaximumAge) {
    const rtype::ecs::Tick thisRun = 4'000'000'000;
    rtype::ecs::Tick tick = 1;  // older than kMaxChangeAge seen from thisRun
    rtype::ecs::clampTick(tick, thisRun);
    EXPECT_EQ(thisRun - tick, rtype::ecs::kMaxChangeAge);
}

TEST(Tick, AClampedTickStaysOlderThanRecentOnes) {
    const rtype::ecs::Tick thisRun = 4'000'000'000;
    rtype::ecs::Tick old = 1;
    rtype::ecs::clampTick(old, thisRun);
    const rtype::ecs::Tick lastRun = thisRun - 100;
    EXPECT_FALSE(rtype::ecs::isNewerThan(old, lastRun, thisRun));
    EXPECT_TRUE(rtype::ecs::isNewerThan(thisRun - 50, lastRun, thisRun));
}
