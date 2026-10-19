/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SchedulerFrameTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>

#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Scheduler.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/FixedTimestep.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::frame {
/// @brief The phases run, one digit each in run order (1 Startup ... 6 Render).
struct Trace {
    int digits;
};
RTYPE_ECS_RESOURCE(Trace, "test.frame.Trace", digits);

/// @brief What the systems saw of Time.
struct Seen {
    int ticks;  ///< Time::tick + 1 for each FixedUpdate run, one digit each.
    float fixedDeltaAtStartup;
    float alphaInUpdate;
};
RTYPE_ECS_RESOURCE(Seen, "test.frame.Seen", ticks, fixedDeltaAtStartup, alphaInUpdate);

struct Hit {
    int damage;
};
RTYPE_ECS_EVENT(Hit, "test.frame.Hit", damage);

/// @brief How many Hit events the sender still has to send, one per run.
struct Pending {
    int count;
};
RTYPE_ECS_RESOURCE(Pending, "test.frame.Pending", count);

/// @brief How many Hit events the reader read.
struct Received {
    int count;
};
RTYPE_ECS_RESOURCE(Received, "test.frame.Received", count);
}  // namespace rtype::test::frame

namespace {
using rtype::ecs::EventReader;
using rtype::ecs::EventWriter;
using rtype::ecs::FixedTimestep;
using rtype::ecs::Phase;
using rtype::ecs::Res;
using rtype::ecs::ResMut;
using rtype::ecs::RunMode;
using rtype::ecs::Scheduler;
using rtype::ecs::Side;
using rtype::ecs::Time;
using rtype::ecs::World;
using rtype::test::frame::Hit;
using rtype::test::frame::Pending;
using rtype::test::frame::Received;
using rtype::test::frame::Seen;
using rtype::test::frame::Trace;
using std::chrono::milliseconds;

void traceStartup(ResMut<Trace> trace) { trace->digits = (trace->digits * 10) + 1; }
void tracePreUpdate(ResMut<Trace> trace) { trace->digits = (trace->digits * 10) + 2; }
void traceFixedUpdate(ResMut<Trace> trace) { trace->digits = (trace->digits * 10) + 3; }
void traceUpdate(ResMut<Trace> trace) { trace->digits = (trace->digits * 10) + 4; }
void tracePostUpdate(ResMut<Trace> trace) { trace->digits = (trace->digits * 10) + 5; }
void traceRender(ResMut<Trace> trace) { trace->digits = (trace->digits * 10) + 6; }

void seeFixedDelta(Res<Time> time, ResMut<Seen> seen) { seen->fixedDeltaAtStartup = time->fixedDelta; }
void seeTick(Res<Time> time, ResMut<Seen> seen) { seen->ticks = (seen->ticks * 10) + static_cast<int>(time->tick) + 1; }
void seeAlpha(Res<Time> time, ResMut<Seen> seen) { seen->alphaInUpdate = time->alpha; }

void sendPending(EventWriter<Hit> hits, ResMut<Pending> pending) {
    if (pending->count > 0) {
        --pending->count;
        hits.send(Hit{.damage = 1});
    }
}

void receive(EventReader<Hit> hits, ResMut<Received> received) {
    for (const Hit& hit : hits) {
        received->count += hit.damage;
    }
}

void writeNothing(EventWriter<Hit> /*hits*/) {}

/// @return A world with one Hit to send, and a count of those read.
World makeEventWorld() {
    World world;
    world.getResources().insert(Pending{.count = 1});
    world.getResources().insert(Received{.count = 0});
    return world;
}

/// @brief A world whose scheduler traces every phase it runs.
class SchedulerFrame : public ::testing::Test {
  protected:
    void SetUp() override {
        world.getResources().insert(Trace{.digits = 0});
        scheduler.addSystem(Phase::kStartup, "startup", &traceStartup);
        scheduler.addSystem(Phase::kPreUpdate, "preUpdate", &tracePreUpdate);
        scheduler.addSystem(Phase::kFixedUpdate, "fixedUpdate", &traceFixedUpdate);
        scheduler.addSystem(Phase::kUpdate, "update", &traceUpdate);
        scheduler.addSystem(Phase::kPostUpdate, "postUpdate", &tracePostUpdate);
        scheduler.addSystem(Phase::kRender, "render", &traceRender);
    }

    /// @return The phases run since the last call, then forgets them.
    int takeTrace() {
        const int digits = world.getResources().require<Trace>().digits;
        world.getResources().requireMut<Trace>()->digits = 0;
        return digits;
    }

    World world;                                       ///< The world the systems run on.
    Scheduler scheduler{world, RunMode::kStandalone};  ///< The scheduler under test.
};
}  // namespace

TEST_F(SchedulerFrame, RunsStartupOnTheFirstFrameOnly) {
    scheduler.runFrame(milliseconds{20});
    EXPECT_EQ(takeTrace(), 123456);  // one fixed tick
    scheduler.runFrame(milliseconds{20});
    EXPECT_EQ(takeTrace(), 23456);
}

TEST_F(SchedulerFrame, RunsFixedUpdateOncePerFixedTick) {
    scheduler.runFrame(milliseconds{50});
    EXPECT_EQ(takeTrace(), 12333456);  // 3 fixed ticks, between PreUpdate and Update
}

TEST_F(SchedulerFrame, SkipsFixedUpdateWhenNoTickIsDue) {
    scheduler.runFrame(milliseconds{5});
    EXPECT_EQ(takeTrace(), 12456);
    scheduler.runFrame(milliseconds{5});
    scheduler.runFrame(milliseconds{5});
    EXPECT_EQ(takeTrace(), 24562456);
    scheduler.runFrame(milliseconds{5});  // 20 ms: the first tick
    EXPECT_EQ(takeTrace(), 23456);
}

TEST_F(SchedulerFrame, CapsFixedUpdatePerFrame) {
    scheduler.setFixedTimestep(FixedTimestep{30, 2});
    scheduler.runFrame(std::chrono::seconds{1});  // 30 ticks due: 2 run
    EXPECT_EQ(takeTrace(), 1233456);
}

TEST(SchedulerTime, IsInsertedAndKeptUpToDate) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    EXPECT_FALSE(world.getResources().has<Time>());
    scheduler.runFrame(milliseconds{20});
    const Time& time = world.getResources().require<Time>();
    EXPECT_FLOAT_EQ(time.delta, 0.02F);
    EXPECT_FLOAT_EQ(time.fixedDelta, 1.F / 60.F);
    EXPECT_EQ(time.tick, 1U);
    EXPECT_FLOAT_EQ(time.alpha, 0.2F);  // 20 ms: one tick, 3.33 ms into the next
    scheduler.runFrame(milliseconds{50});
    EXPECT_EQ(world.getResources().require<Time>().tick, 4U);
}

TEST(SchedulerTime, FollowsTheFixedTimestep) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.setFixedTimestep(FixedTimestep{30, 5});
    EXPECT_EQ(scheduler.getFixedTimestep().getRate(), 30U);
    scheduler.runFrame(milliseconds{50});
    EXPECT_FLOAT_EQ(world.getResources().require<Time>().fixedDelta, 1.F / 30.F);
    EXPECT_EQ(world.getResources().require<Time>().tick, 1U);
}

TEST(SchedulerTime, CountsANegativeFrameAsZero) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.runFrame(milliseconds{-20});
    EXPECT_EQ(world.getResources().require<Time>().delta, 0.F);
    EXPECT_EQ(world.getResources().require<Time>().tick, 0U);
}

TEST(SchedulerTime, IsSeenBySystems) {
    World world;
    world.getResources().insert(Seen{.ticks = 0, .fixedDeltaAtStartup = 0.F, .alphaInUpdate = 0.F});
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kStartup, "seeFixedDelta", &seeFixedDelta);
    scheduler.addSystem(Phase::kFixedUpdate, "seeTick", &seeTick);
    scheduler.addSystem(Phase::kUpdate, "seeAlpha", &seeAlpha);
    scheduler.runFrame(milliseconds{50});
    const Seen& seen = world.getResources().require<Seen>();
    EXPECT_FLOAT_EQ(seen.fixedDeltaAtStartup, 1.F / 60.F);  // Time is there before Startup
    EXPECT_EQ(seen.ticks, 123);                             // ticks 0, 1 and 2, each seen while it runs
    EXPECT_NEAR(seen.alphaInUpdate, 0.F, 1e-6F);            // 50 ms is exactly 3 ticks
}

TEST(SchedulerEvents, KeepsFixedUpdateEventsForTwoFixedTicks) {
    World world = makeEventWorld();
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kFixedUpdate, "send", &sendPending);
    scheduler.runFrame(milliseconds{20});  // a tick: sent, then the queue moves forward
    EXPECT_EQ(world.getEvents().getCount<Hit>(), 1U);
    scheduler.runFrame(milliseconds{5});  // no tick: the end of the frame leaves it alone
    EXPECT_EQ(world.getEvents().getCount<Hit>(), 1U);
    scheduler.runFrame(milliseconds{10});  // the second tick since it was sent: dropped
    EXPECT_EQ(world.getEvents().getCount<Hit>(), 0U);
}

TEST(SchedulerEvents, KeepsOtherEventsForTwoFrames) {
    World world = makeEventWorld();
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "send", &sendPending);
    scheduler.addSystem(Phase::kUpdate, "receive", &receive).before("send");
    scheduler.runFrame(milliseconds{50});  // sent after the reader ran
    EXPECT_EQ(world.getResources().require<Received>().count, 0);
    scheduler.runFrame(milliseconds{50});  // 3 fixed ticks run before Update: the event is still there
    EXPECT_EQ(world.getResources().require<Received>().count, 1);
    EXPECT_EQ(world.getEvents().getCount<Hit>(), 0U);  // dropped at the end of its second frame
}

TEST(SchedulerEvents, LetsFixedUpdateReadEventsSentBeforeIt) {
    World world = makeEventWorld();
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kPreUpdate, "send", &sendPending);
    scheduler.addSystem(Phase::kFixedUpdate, "receive", &receive);
    scheduler.runFrame(milliseconds{5});   // sent; no tick this frame
    scheduler.runFrame(milliseconds{15});  // the first tick, in the event's second frame
    EXPECT_EQ(world.getResources().require<Received>().count, 1);
}

TEST(SchedulerEvents, CountsOnlyTheWritersOfThisRunMode) {
    for (const RunMode mode : {RunMode::kServer, RunMode::kClient}) {
        World world;
        Scheduler scheduler{world, mode};
        scheduler.addSystem(Phase::kFixedUpdate, "write", &writeNothing).setSide(Side::kServer);
        world.getEvents().send(Hit{.damage = 1});
        scheduler.runFrame(milliseconds{5});  // no tick
        scheduler.runFrame(milliseconds{5});
        // The server writes Hit in FixedUpdate: no tick ran, so it is kept. The client does not: two frames drop it.
        EXPECT_EQ(world.getEvents().getCount<Hit>(), mode == RunMode::kServer ? 1U : 0U);
    }
}
