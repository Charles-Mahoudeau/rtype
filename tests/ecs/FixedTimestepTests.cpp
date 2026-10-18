/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FixedTimestepTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <vector>

#include "rtype/ecs/time/FixedTimestep.hpp"

namespace {
using rtype::ecs::FixedTimestep;
using std::chrono::milliseconds;

/// @return The ticks each of `frames` frames of `frame` gives.
std::vector<std::uint32_t> ticksOf(FixedTimestep& timestep, milliseconds frame, int frames) {
    std::vector<std::uint32_t> ticks;
    for (int index = 0; index < frames; ++index) {
        ticks.push_back(timestep.advance(frame));
    }
    return ticks;
}

/// @return The sum of `ticks`.
std::uint32_t total(const std::vector<std::uint32_t>& ticks) {
    std::uint32_t sum = 0;
    for (const std::uint32_t count : ticks) {
        sum += count;
    }
    return sum;
}
}  // namespace

TEST(FixedTimestep, DefaultsToSixtyTicksPerSecondAndFivePerFrame) {
    const FixedTimestep timestep;
    EXPECT_EQ(timestep.getRate(), 60U);
    EXPECT_EQ(timestep.getMaxTicksPerFrame(), 5U);
    EXPECT_FLOAT_EQ(timestep.getTickSeconds(), 1.F / 60.F);
    EXPECT_EQ(timestep.getAlpha(), 0.F);
}

TEST(FixedTimestep, FiveMillisecondFramesRunATickEveryFewFrames) {
    FixedTimestep timestep;
    const std::vector<std::uint32_t> ticks = ticksOf(timestep, milliseconds{5}, 200);  // 1 s
    EXPECT_EQ((std::vector<std::uint32_t>{ticks.begin(), ticks.begin() + 4}), (std::vector<std::uint32_t>{0, 0, 0, 1}));
    EXPECT_EQ(total(ticks), 60U);  // exactly 60 in one second
}

TEST(FixedTimestep, SixteenMillisecondFramesSkipATickNowAndThen) {
    FixedTimestep timestep;
    const std::vector<std::uint32_t> ticks = ticksOf(timestep, milliseconds{16}, 60);  // 0.96 s
    EXPECT_EQ(total(ticks), 57U);                                                      // 0.96 s * 60
    for (const std::uint32_t count : ticks) {
        EXPECT_LE(count, 1U);  // a 16 ms frame is shorter than a tick: never two at once
    }
}

TEST(FixedTimestep, FiftyMillisecondFramesRunThreeTicksEach) {
    FixedTimestep timestep;
    const std::vector<std::uint32_t> ticks = ticksOf(timestep, milliseconds{50}, 20);  // 1 s
    EXPECT_EQ(ticks, std::vector<std::uint32_t>(20, 3));
    EXPECT_EQ(total(ticks), 60U);
}

TEST(FixedTimestep, TheCapStopsASpiralOfDeath) {
    FixedTimestep timestep;
    EXPECT_EQ(timestep.advance(std::chrono::seconds{1}), 5U);  // 60 ticks due: only 5 run, the rest is dropped
    EXPECT_LT(timestep.getAlpha(), 1.F);
    EXPECT_EQ(timestep.advance(milliseconds{0}), 0U);  // nothing left to catch up
}

TEST(FixedTimestep, TellsHowFarTheFrameIsIntoTheNextTick) {
    FixedTimestep timestep;
    EXPECT_EQ(timestep.advance(milliseconds{8}), 0U);
    EXPECT_FLOAT_EQ(timestep.getAlpha(), 0.48F);  // 8 ms of a 16.67 ms tick
    EXPECT_EQ(timestep.advance(milliseconds{10}), 1U);
    EXPECT_FLOAT_EQ(timestep.getAlpha(), 0.08F);  // 18 ms: one tick, 1.33 ms into the next
}

TEST(FixedTimestep, RunsAtTheConfiguredRate) {
    FixedTimestep timestep{30, 2};
    EXPECT_FLOAT_EQ(timestep.getTickSeconds(), 1.F / 30.F);
    EXPECT_EQ(timestep.advance(milliseconds{50}), 1U);  // 1.5 ticks
    EXPECT_FLOAT_EQ(timestep.getAlpha(), 0.5F);
    EXPECT_EQ(timestep.advance(milliseconds{500}), 2U);  // 15.5 ticks due: capped at 2
}

TEST(FixedTimestep, IgnoresANegativeFrame) {
    FixedTimestep timestep;
    EXPECT_EQ(timestep.advance(milliseconds{-20}), 0U);
    EXPECT_EQ(timestep.getAlpha(), 0.F);
}
