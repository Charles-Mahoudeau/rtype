/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TimeClockTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/time/TimeClock.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::time_clock {
/// @brief What the systems saw of Time.
struct Seen {
    int ticks;  ///< Time::tick + 1 for each FixedUpdate run, one digit each.
    float fixedDeltaAtStartup;
    float alphaInUpdate;
};
RTYPE_ECS_RESOURCE(Seen, "test.timeClock.Seen", ticks, fixedDeltaAtStartup, alphaInUpdate);
}  // namespace rtype::test::time_clock

namespace {
using rtype::ecs::FixedTimestep;
using rtype::ecs::Phase;
using rtype::ecs::Res;
using rtype::ecs::ResMut;
using rtype::ecs::RunMode;
using rtype::ecs::Schedules;
using rtype::ecs::Time;
using rtype::ecs::TimeClock;
using rtype::ecs::World;
using rtype::test::time_clock::Seen;
using std::chrono::milliseconds;

void seeFixedDelta(Res<Time> time, ResMut<Seen> seen) { seen->fixedDeltaAtStartup = time->fixedDelta; }
void seeTick(Res<Time> time, ResMut<Seen> seen) { seen->ticks = (seen->ticks * 10) + static_cast<int>(time->tick) + 1; }
void seeAlpha(Res<Time> time, ResMut<Seen> seen) { seen->alphaInUpdate = time->alpha; }

/// @brief Installs a clock whose frames all last `frame`.
void useManualFrames(Schedules& schedules, milliseconds frame, FixedTimestep timestep = FixedTimestep{}) {
    schedules.setClock(std::make_unique<TimeClock>(timestep, frame));
}
}  // namespace

TEST(TimeClock, KeepsItsSettings) {
    const TimeClock real;
    EXPECT_EQ(real.getFixedTimestep().getRate(), 60U);
    EXPECT_FALSE(real.getManualFrame());
    const TimeClock manual{FixedTimestep{30, 2}, milliseconds{16}};
    EXPECT_EQ(manual.getFixedTimestep().getRate(), 30U);
    EXPECT_EQ(manual.getManualFrame(), milliseconds{16});
}

TEST(TimeClock, GivesTheTicksOfEachManualFrame) {
    World world;
    TimeClock clock{FixedTimestep{}, milliseconds{50}};
    EXPECT_EQ(clock.startFrame(world), 3U);
    EXPECT_EQ(clock.startFrame(world), 3U);
}

TEST(TimeClock, CountsTheFirstRealFrameAsZero) {
    World world;
    TimeClock clock;
    EXPECT_EQ(clock.startFrame(world), 0U);
    EXPECT_EQ(world.getResources().require<Time>().delta, 0.F);
    static_cast<void>(clock.startFrame(world));  // some real time: whatever it is, never negative
    EXPECT_GE(world.getResources().require<Time>().delta, 0.F);
}

TEST(TimeClock, InsertsTimeAndKeepsItUpToDate) {
    World world;
    Schedules schedules{world, RunMode::kStandalone};
    useManualFrames(schedules, milliseconds{20});
    EXPECT_FALSE(world.getResources().has<Time>());
    schedules.runFrame();
    const Time& time = world.getResources().require<Time>();
    EXPECT_FLOAT_EQ(time.delta, 0.02F);
    EXPECT_FLOAT_EQ(time.fixedDelta, 1.F / 60.F);
    EXPECT_EQ(time.tick, 1U);
    EXPECT_FLOAT_EQ(time.alpha, 0.2F);  // 20 ms: one tick, 3.33 ms into the next
    schedules.runFrame();               // 40 ms: 2.4 ticks
    EXPECT_EQ(world.getResources().require<Time>().tick, 2U);
}

TEST(TimeClock, FollowsItsFixedTimestep) {
    World world;
    Schedules schedules{world, RunMode::kStandalone};
    useManualFrames(schedules, milliseconds{50}, FixedTimestep{30, 5});
    schedules.runFrame();
    EXPECT_FLOAT_EQ(world.getResources().require<Time>().fixedDelta, 1.F / 30.F);
    EXPECT_EQ(world.getResources().require<Time>().tick, 1U);  // 1.5 ticks
}

TEST(TimeClock, CountsANegativeFrameAsZero) {
    World world;
    Schedules schedules{world, RunMode::kStandalone};
    useManualFrames(schedules, milliseconds{-20});
    schedules.runFrame();
    EXPECT_EQ(world.getResources().require<Time>().delta, 0.F);
    EXPECT_EQ(world.getResources().require<Time>().tick, 0U);
}

TEST(TimeClock, IsSeenBySystems) {
    World world;
    world.getResources().insert(Seen{.ticks = 0, .fixedDeltaAtStartup = 0.F, .alphaInUpdate = 0.F});
    Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kStartup, "seeFixedDelta", &seeFixedDelta);
    schedules.addSystem(Phase::kFixedUpdate, "seeTick", &seeTick);
    schedules.addSystem(Phase::kUpdate, "seeAlpha", &seeAlpha);
    useManualFrames(schedules, milliseconds{50});
    schedules.runFrame();
    const Seen& seen = world.getResources().require<Seen>();
    EXPECT_FLOAT_EQ(seen.fixedDeltaAtStartup, 1.F / 60.F);  // Time is there before Startup
    EXPECT_EQ(seen.ticks, 123);                             // ticks 0, 1 and 2, each seen while it runs
    EXPECT_NEAR(seen.alphaInUpdate, 0.F, 1e-6F);            // 50 ms is exactly 3 ticks
}
