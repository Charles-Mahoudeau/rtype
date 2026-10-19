/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SchedulesFrameTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/FrameClock.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::schedules_frame {
/// @brief What ran, in order: 1 to 6 for the phases (Startup ... Render), 7 to 9 for the clock's calls.
struct Trace {
    std::vector<int> steps;
};
RTYPE_ECS_RESOURCE(Trace, "test.schedulesFrame.Trace");

struct Hit {
    int damage;
};
RTYPE_ECS_EVENT(Hit, "test.schedulesFrame.Hit", damage);

/// @brief How many Hit events the sender still has to send, one per run.
struct Pending {
    int count;
};
RTYPE_ECS_RESOURCE(Pending, "test.schedulesFrame.Pending", count);

/// @brief How many Hit events the reader read.
struct Received {
    int count;
};
RTYPE_ECS_RESOURCE(Received, "test.schedulesFrame.Received", count);
}  // namespace rtype::test::schedules_frame

namespace {
using rtype::ecs::EventReader;
using rtype::ecs::EventWriter;
using rtype::ecs::FrameClock;
using rtype::ecs::Phase;
using rtype::ecs::ResMut;
using rtype::ecs::RunMode;
using rtype::ecs::Schedules;
using rtype::ecs::Side;
using rtype::ecs::World;
using rtype::ecs::exceptions::ScheduleException;
using rtype::test::schedules_frame::Hit;
using rtype::test::schedules_frame::Pending;
using rtype::test::schedules_frame::Received;
using rtype::test::schedules_frame::Trace;

constexpr int kStartFrame = 7;       ///< The clock's startFrame() in a trace.
constexpr int kFinishFixedTick = 8;  ///< The clock's finishFixedTick() in a trace.
constexpr int kStartUpdate = 9;      ///< The clock's startUpdate() in a trace.

void traceStartup(ResMut<Trace> trace) { trace->steps.push_back(1); }
void tracePreUpdate(ResMut<Trace> trace) { trace->steps.push_back(2); }
void traceFixedUpdate(ResMut<Trace> trace) { trace->steps.push_back(3); }
void traceUpdate(ResMut<Trace> trace) { trace->steps.push_back(4); }
void tracePostUpdate(ResMut<Trace> trace) { trace->steps.push_back(5); }
void traceRender(ResMut<Trace> trace) { trace->steps.push_back(6); }

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

/// @brief A clock giving each frame the number of fixed ticks of a script (0 once it is over), and tracing its calls
/// when the world has a Trace.
class ScriptedClock : public FrameClock {
  public:
    explicit ScriptedClock(std::vector<std::uint32_t> ticks) : _ticks{std::move(ticks)} {}
    ~ScriptedClock() override = default;
    ScriptedClock(const ScriptedClock& other) = delete;
    ScriptedClock& operator=(const ScriptedClock& other) = delete;
    ScriptedClock(ScriptedClock&& other) = delete;
    ScriptedClock& operator=(ScriptedClock&& other) = delete;

    std::uint32_t startFrame(World& world) override {
        trace(world, kStartFrame);
        const std::uint32_t ticks = _frame < _ticks.size() ? _ticks.at(_frame) : 0;
        ++_frame;
        return ticks;
    }

    void finishFixedTick(World& world) override { trace(world, kFinishFixedTick); }

    void startUpdate(World& world) override { trace(world, kStartUpdate); }

  private:
    static void trace(World& world, int step) {
        if (world.getResources().has<Trace>()) {
            world.getResources().requireMut<Trace>()->steps.push_back(step);
        }
    }

    std::vector<std::uint32_t> _ticks;  ///< The fixed ticks of each frame.
    std::size_t _frame{0};              ///< The frames started so far.
};

/// @return Schedules running `ticks` fixed ticks in each frame, in order.
void useTicks(Schedules& schedules, std::vector<std::uint32_t> ticks) {
    schedules.setClock(std::make_unique<ScriptedClock>(std::move(ticks)));
}

/// @return A world with one Hit to send, and a count of those read.
World makeEventWorld() {
    World world;
    world.getResources().insert(Pending{.count = 1});
    world.getResources().insert(Received{.count = 0});
    return world;
}

/// @brief A world whose schedules trace every phase they run.
class SchedulesFrame : public ::testing::Test {
  protected:
    void SetUp() override {
        world.getResources().insert(Trace{});
        schedules.addSystem(Phase::kStartup, "startup", &traceStartup);
        schedules.addSystem(Phase::kPreUpdate, "preUpdate", &tracePreUpdate);
        schedules.addSystem(Phase::kFixedUpdate, "fixedUpdate", &traceFixedUpdate);
        schedules.addSystem(Phase::kUpdate, "update", &traceUpdate);
        schedules.addSystem(Phase::kPostUpdate, "postUpdate", &tracePostUpdate);
        schedules.addSystem(Phase::kRender, "render", &traceRender);
    }

    /// @return What ran since the last call, then forgets it.
    std::vector<int> takeTrace() { return std::exchange(world.getResources().requireMut<Trace>()->steps, {}); }

    World world;                                       ///< The world the systems run on.
    Schedules schedules{world, RunMode::kStandalone};  ///< The schedules under test.
};
}  // namespace

TEST_F(SchedulesFrame, RunsStartupOnTheFirstFrameOnly) {
    useTicks(schedules, {1, 1});
    schedules.runFrame();
    EXPECT_EQ(takeTrace(), (std::vector<int>{7, 1, 2, 3, 8, 9, 4, 5, 6}));
    schedules.runFrame();
    EXPECT_EQ(takeTrace(), (std::vector<int>{7, 2, 3, 8, 9, 4, 5, 6}));
}

TEST_F(SchedulesFrame, RunsFixedUpdateOncePerTick) {
    useTicks(schedules, {3});
    schedules.runFrame();
    EXPECT_EQ(takeTrace(), (std::vector<int>{7, 1, 2, 3, 8, 3, 8, 3, 8, 9, 4, 5, 6}));
}

TEST_F(SchedulesFrame, SkipsFixedUpdateWhenNoTickIsDue) {
    useTicks(schedules, {0});
    schedules.runFrame();
    EXPECT_EQ(takeTrace(), (std::vector<int>{7, 1, 2, 9, 4, 5, 6}));
}

TEST_F(SchedulesFrame, NeverRunsFixedUpdateWithoutAClock) {
    schedules.runFrame();
    schedules.runFrame();
    EXPECT_EQ(takeTrace(), (std::vector<int>{1, 2, 4, 5, 6, 2, 4, 5, 6}));
}

TEST_F(SchedulesFrame, BuildsBeforeTheFrameStarts) {
    useTicks(schedules, {1, 1});
    schedules.addSystem(Phase::kUpdate, "broken", &tracePreUpdate).after("missing");
    EXPECT_THROW(schedules.runFrame(), ScheduleException);
    EXPECT_TRUE(takeTrace().empty());  // not even the clock: the frame did not start
}

TEST(SchedulesEvents, KeepsFixedUpdateEventsForTwoFixedTicks) {
    World world = makeEventWorld();
    Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kFixedUpdate, "send", &sendPending);
    useTicks(schedules, {1, 0, 1});
    schedules.runFrame();  // a tick: sent, then the queue moves forward
    EXPECT_EQ(world.getEvents().getCount<Hit>(), 1U);
    schedules.runFrame();  // no tick: the end of the frame leaves it alone
    EXPECT_EQ(world.getEvents().getCount<Hit>(), 1U);
    schedules.runFrame();  // the second tick since it was sent: dropped
    EXPECT_EQ(world.getEvents().getCount<Hit>(), 0U);
}

TEST(SchedulesEvents, KeepsOtherEventsForTwoFrames) {
    World world = makeEventWorld();
    Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kUpdate, "send", &sendPending);
    schedules.addSystem(Phase::kUpdate, "receive", &receive).before("send");
    useTicks(schedules, {3, 3});
    schedules.runFrame();  // sent after the reader ran
    EXPECT_EQ(world.getResources().require<Received>().count, 0);
    schedules.runFrame();  // 3 fixed ticks run before Update: the event is still there
    EXPECT_EQ(world.getResources().require<Received>().count, 1);
    EXPECT_EQ(world.getEvents().getCount<Hit>(), 0U);  // dropped at the end of its second frame
}

TEST(SchedulesEvents, LetsFixedUpdateReadEventsSentBeforeIt) {
    World world = makeEventWorld();
    Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kPreUpdate, "send", &sendPending);
    schedules.addSystem(Phase::kFixedUpdate, "receive", &receive);
    useTicks(schedules, {0, 1});
    schedules.runFrame();  // sent; no tick this frame
    schedules.runFrame();  // the first tick, in the event's second frame
    EXPECT_EQ(world.getResources().require<Received>().count, 1);
}

TEST(SchedulesEvents, CountsOnlyTheWritersOfThisRunMode) {
    for (const RunMode mode : {RunMode::kServer, RunMode::kClient}) {
        World world;
        Schedules schedules{world, mode};
        schedules.addSystem(Phase::kFixedUpdate, "write", &writeNothing).setSide(Side::kServer);
        world.getEvents().send(Hit{.damage = 1});
        useTicks(schedules, {0, 0});
        schedules.runFrame();
        schedules.runFrame();
        // The server writes Hit in FixedUpdate: no tick ran, so it is kept. The client does not: two frames drop it.
        EXPECT_EQ(world.getEvents().getCount<Hit>(), mode == RunMode::kServer ? 1U : 0U);
    }
}
