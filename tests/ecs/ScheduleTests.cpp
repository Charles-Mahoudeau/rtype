/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScheduleTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedule.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::schedule {
struct Marker {};
RTYPE_ECS_COMPONENT(Marker, "test.schedule.Marker");

struct ServerRuns {
    int count;
};
RTYPE_ECS_RESOURCE(ServerRuns, "test.schedule.ServerRuns", count);

struct ClientRuns {
    int count;
};
RTYPE_ECS_RESOURCE(ClientRuns, "test.schedule.ClientRuns", count);

struct BothRuns {
    int count;
};
RTYPE_ECS_RESOURCE(BothRuns, "test.schedule.BothRuns", count);

struct SeenInPhase {
    int count;
};
RTYPE_ECS_RESOURCE(SeenInPhase, "test.schedule.SeenInPhase", count);

struct SeenAfterPhase {
    int count;
};
RTYPE_ECS_RESOURCE(SeenAfterPhase, "test.schedule.SeenAfterPhase", count);

struct Health {
    int value;
};
RTYPE_ECS_COMPONENT(Health, "test.schedule.Health", value);

struct Score {
    int value;
};
RTYPE_ECS_RESOURCE(Score, "test.schedule.Score", value);

struct Hit {
    int damage;
};
RTYPE_ECS_EVENT(Hit, "test.schedule.Hit", damage);

struct Heal {
    int amount;
};
RTYPE_ECS_EVENT(Heal, "test.schedule.Heal", amount);
}  // namespace rtype::test::schedule

namespace {
using rtype::ecs::Commands;
using rtype::ecs::EventId;
using rtype::ecs::Phase;
using rtype::ecs::Query;
using rtype::ecs::ResMut;
using rtype::ecs::RunMode;
using rtype::ecs::Schedule;
using rtype::ecs::Side;
using rtype::ecs::World;
using rtype::ecs::exceptions::ScheduleException;
using rtype::test::schedule::BothRuns;
using rtype::test::schedule::ClientRuns;
using rtype::test::schedule::Heal;
using rtype::test::schedule::Hit;
using rtype::test::schedule::Marker;
using rtype::test::schedule::SeenAfterPhase;
using rtype::test::schedule::SeenInPhase;
using rtype::test::schedule::ServerRuns;

void noop() {}

void countServer(ResMut<ServerRuns> runs) { ++runs->count; }
void countClient(ResMut<ClientRuns> runs) { ++runs->count; }
void countBoth(ResMut<BothRuns> runs) { ++runs->count; }

void spawnMarker(Commands& commands) { static_cast<void>(commands.spawn().add(Marker{})); }

void countInPhase(Query<rtype::ecs::With<Marker>> markers, ResMut<SeenInPhase> seen) {
    for (auto [entity] : markers) {
        ++seen->count;
    }
}

void countAfterPhase(Query<rtype::ecs::With<Marker>> markers, ResMut<SeenAfterPhase> seen) {
    for (auto [entity] : markers) {
        ++seen->count;
    }
}

void writeHealth(Query<rtype::test::schedule::Health> /*healths*/) {}
void readHealth(Query<const rtype::test::schedule::Health> /*healths*/) {}
void writeScore(ResMut<rtype::test::schedule::Score> /*score*/) {}
void readScore(rtype::ecs::Res<rtype::test::schedule::Score> /*score*/) {}
void sendHit(rtype::ecs::EventWriter<Hit> /*hits*/) {}
void readHit(rtype::ecs::EventReader<Hit> /*hits*/) {}
void sendHeal(rtype::ecs::EventWriter<Heal> /*heals*/) {}

/// @return The order of a schedule, as strings, to compare it.
std::vector<std::string> orderOf(const Schedule& schedule) {
    std::vector<std::string> names;
    for (const std::string_view name : schedule.getOrder()) {
        names.emplace_back(name);
    }
    return names;
}

/// @brief Expects build() to throw a ScheduleException whose message contains every text of `texts`.
void expectBuildError(Schedule& schedule, const std::vector<std::string>& texts) {
    try {
        schedule.build();
        FAIL() << "build() did not throw";
    } catch (const ScheduleException& error) {
        for (const std::string& text : texts) {
            EXPECT_NE(std::string_view{error.what()}.find(text), std::string_view::npos) << error.what();
        }
    }
}

/// @return The events of the schedule's list, as a vector to compare it.
std::vector<EventId> writtenBy(const Schedule& schedule) {
    const std::span<const EventId> events = schedule.getWrittenEvents();
    return {events.begin(), events.end()};
}
}  // namespace

TEST(Schedule, KnowsItsPhaseAndMode) {
    World world;
    const Schedule schedule{world, RunMode::kClient, Phase::kRender};
    EXPECT_EQ(schedule.getPhase(), Phase::kRender);
    EXPECT_EQ(schedule.getRunMode(), RunMode::kClient);
    EXPECT_FALSE(schedule.isBuilt());
    EXPECT_TRUE(schedule.getOrder().empty());
}

TEST(Schedule, RunsSystemsWithoutConstraintsByName) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("c", &noop);
    schedule.addSystem("a", &noop);
    schedule.addSystem("b", &noop);
    schedule.build();
    EXPECT_EQ(orderOf(schedule), (std::vector<std::string>{"a", "b", "c"}));
}

TEST(Schedule, FollowsAfterAndBefore) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kFixedUpdate};
    schedule.addSystem("a.damage", &noop).after("z.collision");
    schedule.addSystem("z.collision", &noop).after("m.movement");
    schedule.addSystem("m.movement", &noop);
    schedule.addSystem("b.input", &noop).before("m.movement");
    schedule.build();
    EXPECT_EQ(orderOf(schedule), (std::vector<std::string>{"b.input", "m.movement", "z.collision", "a.damage"}));
}

TEST(Schedule, OrdersTheSameWhateverTheRegistrationOrder) {
    World world;
    Schedule forward{world, RunMode::kStandalone, Phase::kUpdate};
    forward.addSystem("a", &noop);
    forward.addSystem("b", &noop).after("d");
    forward.addSystem("c", &noop);
    forward.addSystem("d", &noop);
    Schedule backward{world, RunMode::kStandalone, Phase::kUpdate};
    backward.addSystem("d", &noop);
    backward.addSystem("c", &noop);
    backward.addSystem("b", &noop).after("d");
    backward.addSystem("a", &noop);
    forward.build();
    backward.build();
    EXPECT_EQ(orderOf(forward), (std::vector<std::string>{"a", "c", "d", "b"}));
    EXPECT_EQ(orderOf(forward), orderOf(backward));
}

TEST(Schedule, RejectsACycleListingItsSystems) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kFixedUpdate};
    schedule.addSystem("a", &noop).after("c");
    schedule.addSystem("b", &noop).after("a");
    schedule.addSystem("c", &noop).after("b");
    schedule.addSystem("d", &noop).after("c");  // waits on the cycle, not part of it
    expectBuildError(schedule, {"cycle", "FixedUpdate", "'a'", "'b'", "'c'"});
}

TEST(Schedule, RejectsASystemOrderedAfterItself) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("loop", &noop).after("loop");
    expectBuildError(schedule, {"cycle", "'loop' -> 'loop'"});
}

TEST(Schedule, RejectsATargetOutsideTheSchedule) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("rtype.collision", &noop).after("rtype.movment");
    expectBuildError(schedule, {"'rtype.collision' runs after 'rtype.movment', which is not a system of phase Update"});
}

TEST(Schedule, RejectsTwoSystemsWithTheSameName) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("rtype.movement", &noop);
    EXPECT_TRUE(schedule.contains("rtype.movement"));
    EXPECT_FALSE(schedule.contains("rtype.collision"));
    EXPECT_THROW(schedule.addSystem("rtype.movement", &noop), ScheduleException);
    schedule.build();
    EXPECT_EQ(orderOf(schedule), (std::vector<std::string>{"rtype.movement"}));  // the second was not added
}

TEST(Schedule, RunsOnlyTheSystemsOfItsMode) {
    for (const RunMode mode : {RunMode::kServer, RunMode::kClient, RunMode::kStandalone}) {
        World world;
        world.getResources().insert(ServerRuns{.count = 0});
        world.getResources().insert(ClientRuns{.count = 0});
        world.getResources().insert(BothRuns{.count = 0});
        Schedule schedule{world, mode, Phase::kUpdate};
        schedule.addSystem("server", &countServer);  // kServer by default
        schedule.addSystem("client", &countClient).setSide(Side::kClient);
        schedule.addSystem("both", &countBoth).setSide(Side::kBoth);
        schedule.run();
        EXPECT_EQ(world.getResources().get<ServerRuns>()->count, mode == RunMode::kClient ? 0 : 1);
        EXPECT_EQ(world.getResources().get<ClientRuns>()->count, mode == RunMode::kServer ? 0 : 1);
        EXPECT_EQ(world.getResources().get<BothRuns>()->count, 1);
    }
}

TEST(Schedule, AppliesCommandsAfterItsSystems) {
    World world;
    world.getResources().insert(SeenInPhase{.count = 0});
    world.getResources().insert(SeenAfterPhase{.count = 0});
    Schedule fixed{world, RunMode::kStandalone, Phase::kFixedUpdate};
    fixed.addSystem("spawn", &spawnMarker);
    fixed.addSystem("count", &countInPhase).after("spawn");
    Schedule update{world, RunMode::kStandalone, Phase::kUpdate};
    update.addSystem("later", &countAfterPhase);
    fixed.run();
    EXPECT_EQ(world.getResources().get<SeenInPhase>()->count, 0);  // the spawn waits for the end of the schedule
    EXPECT_EQ(world.getEntityCount(), 1U);
    update.run();
    EXPECT_EQ(world.getResources().get<SeenAfterPhase>()->count, 1);
}

TEST(Schedule, BuildsWhenRunAndAgainAfterChanges) {
    World world;
    world.getResources().insert(ServerRuns{.count = 0});
    world.getResources().insert(ClientRuns{.count = 0});
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("server", &countServer);
    schedule.run();  // built on the first run
    EXPECT_TRUE(schedule.isBuilt());
    EXPECT_EQ(orderOf(schedule), (std::vector<std::string>{"server"}));

    schedule.addSystem("client", &countClient);
    EXPECT_FALSE(schedule.isBuilt());
    schedule.run();  // built again: the new system runs
    EXPECT_EQ(orderOf(schedule), (std::vector<std::string>{"client", "server"}));
    EXPECT_EQ(world.getResources().get<ClientRuns>()->count, 1);
    EXPECT_EQ(world.getResources().get<ServerRuns>()->count, 2);
}

TEST(Schedule, RunThrowsWhenItCannotBeBuilt) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("a", &noop).after("missing");
    EXPECT_THROW(schedule.run(), ScheduleException);
    EXPECT_FALSE(schedule.isBuilt());
}

TEST(Schedule, RejectsTwoWritersOfAComponentWithNoOrder) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kFixedUpdate};
    schedule.addSystem("rtype.damage", &writeHealth);
    schedule.addSystem("rtype.regen", &writeHealth);
    expectBuildError(schedule, {"phase FixedUpdate",
                                "'rtype.damage' and 'rtype.regen' both use component "
                                "'test.schedule.Health', one of them writing it"});
}

TEST(Schedule, RejectsAReaderAndAWriterWithNoOrder) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("ui", &readHealth);
    schedule.addSystem("damage", &writeHealth);
    expectBuildError(schedule, {"'damage' and 'ui' both use component 'test.schedule.Health'"});
}

TEST(Schedule, RejectsConflictsOnResourcesAndEvents) {
    World world;
    Schedule resources{world, RunMode::kStandalone, Phase::kUpdate};
    resources.addSystem("a", &writeScore);
    resources.addSystem("b", &readScore);
    expectBuildError(resources, {"resource 'test.schedule.Score'"});

    Schedule events{world, RunMode::kStandalone, Phase::kUpdate};
    events.addSystem("detect", &sendHit);
    events.addSystem("react", &readHit);
    expectBuildError(events, {"event 'test.schedule.Hit'"});
}

TEST(Schedule, ListsEveryAmbiguousPair) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("a", &writeHealth);
    schedule.addSystem("b", &writeHealth);
    schedule.addSystem("c", &writeScore);
    schedule.addSystem("d", &readScore);
    expectBuildError(schedule, {"'a' and 'b'", "'c' and 'd'"});
}

TEST(Schedule, AcceptsReadersSharingData) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("a", &readHealth);
    schedule.addSystem("b", &readHealth);
    EXPECT_NO_THROW(schedule.build());
}

TEST(Schedule, AcceptsConflictingSystemsOnceOrdered) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kUpdate};
    schedule.addSystem("damage", &writeHealth);
    schedule.addSystem("move", &noop).after("damage");
    schedule.addSystem("regen", &writeHealth).after("move");  // ordered through "move"
    schedule.addSystem("ui", &readHealth).after("regen");
    EXPECT_NO_THROW(schedule.build());
}

TEST(Schedule, ChecksAmbiguitiesWhateverTheRunMode) {
    World world;
    Schedule schedule{world, RunMode::kServer, Phase::kUpdate};
    schedule.addSystem("server", &writeHealth);
    schedule.addSystem("client", &writeHealth).setSide(Side::kClient);  // never run on a server
    expectBuildError(schedule, {"'client' and 'server'"});              // they do run together in standalone
}

TEST(Schedule, ListsTheEventsItsSystemsWrite) {
    World world;
    Schedule schedule{world, RunMode::kStandalone, Phase::kFixedUpdate};
    schedule.addSystem("heal", &sendHeal);
    schedule.addSystem("hit", &sendHit);
    schedule.addSystem("hitAgain", &sendHit).after("hit");    // written twice, listed once
    schedule.addSystem("react", &readHit).after("hitAgain");  // reading does not count
    schedule.build();
    std::vector<EventId> expected{*world.getEvents().find("test.schedule.Hit"),
                                  *world.getEvents().find("test.schedule.Heal")};
    std::ranges::sort(expected);
    EXPECT_EQ(writtenBy(schedule), expected);
}

TEST(Schedule, ListsOnlyTheWritersOfItsMode) {
    World world;
    Schedule schedule{world, RunMode::kClient, Phase::kFixedUpdate};
    schedule.addSystem("hit", &sendHit).setSide(Side::kServer);
    schedule.build();
    EXPECT_TRUE(writtenBy(schedule).empty());
}

TEST(Schedule, BuildsAgainAfterASideChange) {
    World world;
    Schedule schedule{world, RunMode::kClient, Phase::kFixedUpdate};
    Schedule::SystemConfig hit = schedule.addSystem("hit", &sendHit);
    schedule.build();
    EXPECT_TRUE(writtenBy(schedule).empty());  // kServer by default: not run on a client
    hit.setSide(Side::kBoth);
    EXPECT_FALSE(schedule.isBuilt());
    schedule.build();
    EXPECT_EQ(writtenBy(schedule).size(), 1U);
}
