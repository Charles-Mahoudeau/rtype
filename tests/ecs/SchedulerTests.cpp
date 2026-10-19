/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SchedulerTests
*/

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Scheduler.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::scheduler {
struct Marker {};
RTYPE_ECS_COMPONENT(Marker, "test.scheduler.Marker");

struct ServerRuns {
    int count;
};
RTYPE_ECS_RESOURCE(ServerRuns, "test.scheduler.ServerRuns", count);

struct ClientRuns {
    int count;
};
RTYPE_ECS_RESOURCE(ClientRuns, "test.scheduler.ClientRuns", count);

struct BothRuns {
    int count;
};
RTYPE_ECS_RESOURCE(BothRuns, "test.scheduler.BothRuns", count);

struct SeenInPhase {
    int count;
};
RTYPE_ECS_RESOURCE(SeenInPhase, "test.scheduler.SeenInPhase", count);

struct SeenAfterPhase {
    int count;
};
RTYPE_ECS_RESOURCE(SeenAfterPhase, "test.scheduler.SeenAfterPhase", count);
struct Health {
    int value;
};
RTYPE_ECS_COMPONENT(Health, "test.scheduler.Health", value);

struct Score {
    int value;
};
RTYPE_ECS_RESOURCE(Score, "test.scheduler.Score", value);

struct Hit {
    int damage;
};
RTYPE_ECS_EVENT(Hit, "test.scheduler.Hit", damage);
}  // namespace rtype::test::scheduler

namespace {
using rtype::ecs::Commands;
using rtype::ecs::Phase;
using rtype::ecs::Query;
using rtype::ecs::ResMut;
using rtype::ecs::RunMode;
using rtype::ecs::Scheduler;
using rtype::ecs::Side;
using rtype::ecs::World;
using rtype::ecs::exceptions::ScheduleException;
using rtype::test::scheduler::BothRuns;
using rtype::test::scheduler::ClientRuns;
using rtype::test::scheduler::Marker;
using rtype::test::scheduler::SeenAfterPhase;
using rtype::test::scheduler::SeenInPhase;
using rtype::test::scheduler::ServerRuns;

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

void writeHealth(Query<rtype::test::scheduler::Health> /*healths*/) {}
void readHealth(Query<const rtype::test::scheduler::Health> /*healths*/) {}
void writeScore(ResMut<rtype::test::scheduler::Score> /*score*/) {}
void readScore(rtype::ecs::Res<rtype::test::scheduler::Score> /*score*/) {}
void sendHit(rtype::ecs::EventWriter<rtype::test::scheduler::Hit> /*hits*/) {}
void readHit(rtype::ecs::EventReader<rtype::test::scheduler::Hit> /*hits*/) {}

/// @return The order of a phase, as strings, to compare it.
std::vector<std::string> orderOf(const Scheduler& scheduler, Phase phase) {
    std::vector<std::string> names;
    for (const std::string_view name : scheduler.getOrder(phase)) {
        names.emplace_back(name);
    }
    return names;
}

/// @brief Expects build() to throw a ScheduleException whose message contains every text of `texts`.
void expectBuildError(Scheduler& scheduler, const std::vector<std::string>& texts) {
    try {
        scheduler.build();
        FAIL() << "build() did not throw";
    } catch (const ScheduleException& error) {
        for (const std::string& text : texts) {
            EXPECT_NE(std::string_view{error.what()}.find(text), std::string_view::npos) << error.what();
        }
    }
}
}  // namespace

TEST(Scheduler, RunsSystemsWithoutConstraintsByName) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "c", &noop);
    scheduler.addSystem(Phase::kUpdate, "a", &noop);
    scheduler.addSystem(Phase::kUpdate, "b", &noop);
    scheduler.build();
    EXPECT_EQ(orderOf(scheduler, Phase::kUpdate), (std::vector<std::string>{"a", "b", "c"}));
}

TEST(Scheduler, FollowsAfterAndBefore) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kFixedUpdate, "a.damage", &noop).after("z.collision");
    scheduler.addSystem(Phase::kFixedUpdate, "z.collision", &noop).after("m.movement");
    scheduler.addSystem(Phase::kFixedUpdate, "m.movement", &noop);
    scheduler.addSystem(Phase::kFixedUpdate, "b.input", &noop).before("m.movement");
    scheduler.build();
    EXPECT_EQ(orderOf(scheduler, Phase::kFixedUpdate),
              (std::vector<std::string>{"b.input", "m.movement", "z.collision", "a.damage"}));
}

TEST(Scheduler, OrdersTheSameWhateverTheRegistrationOrder) {
    World world;
    Scheduler forward{world, RunMode::kStandalone};
    forward.addSystem(Phase::kUpdate, "a", &noop);
    forward.addSystem(Phase::kUpdate, "b", &noop).after("d");
    forward.addSystem(Phase::kUpdate, "c", &noop);
    forward.addSystem(Phase::kUpdate, "d", &noop);
    Scheduler backward{world, RunMode::kStandalone};
    backward.addSystem(Phase::kUpdate, "d", &noop);
    backward.addSystem(Phase::kUpdate, "c", &noop);
    backward.addSystem(Phase::kUpdate, "b", &noop).after("d");
    backward.addSystem(Phase::kUpdate, "a", &noop);
    forward.build();
    backward.build();
    EXPECT_EQ(orderOf(forward, Phase::kUpdate), (std::vector<std::string>{"a", "c", "d", "b"}));
    EXPECT_EQ(orderOf(forward, Phase::kUpdate), orderOf(backward, Phase::kUpdate));
}

TEST(Scheduler, KeepsPhasesApart) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "update", &noop);
    scheduler.addSystem(Phase::kRender, "render", &noop);
    scheduler.build();
    EXPECT_EQ(orderOf(scheduler, Phase::kUpdate), (std::vector<std::string>{"update"}));
    EXPECT_EQ(orderOf(scheduler, Phase::kRender), (std::vector<std::string>{"render"}));
    EXPECT_TRUE(scheduler.getOrder(Phase::kStartup).empty());
}

TEST(Scheduler, RejectsACycleListingItsSystems) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kFixedUpdate, "a", &noop).after("c");
    scheduler.addSystem(Phase::kFixedUpdate, "b", &noop).after("a");
    scheduler.addSystem(Phase::kFixedUpdate, "c", &noop).after("b");
    scheduler.addSystem(Phase::kFixedUpdate, "d", &noop).after("c");  // waits on the cycle, not part of it
    expectBuildError(scheduler, {"cycle", "FixedUpdate", "'a'", "'b'", "'c'"});
}

TEST(Scheduler, RejectsASystemOrderedAfterItself) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "loop", &noop).after("loop");
    expectBuildError(scheduler, {"cycle", "'loop' -> 'loop'"});
}

TEST(Scheduler, RejectsAnUnknownTarget) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "rtype.collision", &noop).after("rtype.movment");
    expectBuildError(scheduler, {"'rtype.collision' runs after 'rtype.movment', which is not a system"});
}

TEST(Scheduler, RejectsATargetInAnotherPhase) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kPreUpdate, "rtype.input", &noop);
    scheduler.addSystem(Phase::kFixedUpdate, "rtype.movement", &noop).before("rtype.input");
    expectBuildError(scheduler, {"'rtype.movement' runs before 'rtype.input', which is in phase PreUpdate, not "
                                 "FixedUpdate"});
}

TEST(Scheduler, RejectsTwoSystemsWithTheSameName) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "rtype.movement", &noop);
    scheduler.addSystem(Phase::kRender, "rtype.movement", &noop);
    expectBuildError(scheduler, {"Two systems are named 'rtype.movement'"});
}

TEST(Scheduler, RunsOnlyTheSystemsOfItsMode) {
    for (const RunMode mode : {RunMode::kServer, RunMode::kClient, RunMode::kStandalone}) {
        World world;
        world.getResources().insert(ServerRuns{.count = 0});
        world.getResources().insert(ClientRuns{.count = 0});
        world.getResources().insert(BothRuns{.count = 0});
        Scheduler scheduler{world, mode};
        scheduler.addSystem(Phase::kUpdate, "server", &countServer);  // kServer by default
        scheduler.addSystem(Phase::kUpdate, "client", &countClient).setSide(Side::kClient);
        scheduler.addSystem(Phase::kUpdate, "both", &countBoth).setSide(Side::kBoth);
        scheduler.run(Phase::kUpdate);
        EXPECT_EQ(world.getResources().get<ServerRuns>()->count, mode == RunMode::kClient ? 0 : 1);
        EXPECT_EQ(world.getResources().get<ClientRuns>()->count, mode == RunMode::kServer ? 0 : 1);
        EXPECT_EQ(world.getResources().get<BothRuns>()->count, 1);
    }
}

TEST(Scheduler, RunsOnlyTheGivenPhase) {
    World world;
    world.getResources().insert(ServerRuns{.count = 0});
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kRender, "server", &countServer);
    scheduler.run(Phase::kUpdate);
    EXPECT_EQ(world.getResources().get<ServerRuns>()->count, 0);
    scheduler.run(Phase::kRender);
    EXPECT_EQ(world.getResources().get<ServerRuns>()->count, 1);
}

TEST(Scheduler, AppliesCommandsAfterThePhase) {
    World world;
    world.getResources().insert(SeenInPhase{.count = 0});
    world.getResources().insert(SeenAfterPhase{.count = 0});
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kFixedUpdate, "spawn", &spawnMarker);
    scheduler.addSystem(Phase::kFixedUpdate, "count", &countInPhase).after("spawn");
    scheduler.addSystem(Phase::kUpdate, "later", &countAfterPhase);
    scheduler.run(Phase::kFixedUpdate);
    EXPECT_EQ(world.getResources().get<SeenInPhase>()->count, 0);  // the spawn waits for the end of the phase
    EXPECT_EQ(world.getEntityCount(), 1U);
    scheduler.run(Phase::kUpdate);
    EXPECT_EQ(world.getResources().get<SeenAfterPhase>()->count, 1);
}

TEST(Scheduler, BuildsWhenRunAndAgainAfterChanges) {
    World world;
    world.getResources().insert(ServerRuns{.count = 0});
    world.getResources().insert(ClientRuns{.count = 0});
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "server", &countServer);
    scheduler.run(Phase::kUpdate);  // built on the first run
    EXPECT_EQ(orderOf(scheduler, Phase::kUpdate), (std::vector<std::string>{"server"}));

    scheduler.addSystem(Phase::kUpdate, "client", &countClient);
    scheduler.run(Phase::kUpdate);  // built again: the new system runs
    EXPECT_EQ(orderOf(scheduler, Phase::kUpdate), (std::vector<std::string>{"client", "server"}));
    EXPECT_EQ(world.getResources().get<ClientRuns>()->count, 1);
    EXPECT_EQ(world.getResources().get<ServerRuns>()->count, 2);
}

TEST(Scheduler, RunThrowsWhenTheScheduleCannotBeBuilt) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "a", &noop).after("missing");
    EXPECT_THROW(scheduler.run(Phase::kUpdate), ScheduleException);
}

TEST(Scheduler, RejectsTwoWritersOfAComponentWithNoOrder) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kFixedUpdate, "rtype.damage", &writeHealth);
    scheduler.addSystem(Phase::kFixedUpdate, "rtype.regen", &writeHealth);
    expectBuildError(scheduler, {"phase FixedUpdate",
                                 "'rtype.damage' and 'rtype.regen' both use component "
                                 "'test.scheduler.Health', one of them writing it"});
}

TEST(Scheduler, RejectsAReaderAndAWriterWithNoOrder) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "ui", &readHealth);
    scheduler.addSystem(Phase::kUpdate, "damage", &writeHealth);
    expectBuildError(scheduler, {"'damage' and 'ui' both use component 'test.scheduler.Health'"});
}

TEST(Scheduler, RejectsConflictsOnResourcesAndEvents) {
    World world;
    Scheduler resources{world, RunMode::kStandalone};
    resources.addSystem(Phase::kUpdate, "a", &writeScore);
    resources.addSystem(Phase::kUpdate, "b", &readScore);
    expectBuildError(resources, {"resource 'test.scheduler.Score'"});

    Scheduler events{world, RunMode::kStandalone};
    events.addSystem(Phase::kUpdate, "detect", &sendHit);
    events.addSystem(Phase::kUpdate, "react", &readHit);
    expectBuildError(events, {"event 'test.scheduler.Hit'"});
}

TEST(Scheduler, ListsEveryAmbiguousPair) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "a", &writeHealth);
    scheduler.addSystem(Phase::kUpdate, "b", &writeHealth);
    scheduler.addSystem(Phase::kUpdate, "c", &writeScore);
    scheduler.addSystem(Phase::kUpdate, "d", &readScore);
    expectBuildError(scheduler, {"'a' and 'b'", "'c' and 'd'"});
}

TEST(Scheduler, AcceptsReadersSharingData) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "a", &readHealth);
    scheduler.addSystem(Phase::kUpdate, "b", &readHealth);
    EXPECT_NO_THROW(scheduler.build());
}

TEST(Scheduler, AcceptsConflictingSystemsOnceOrdered) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kUpdate, "damage", &writeHealth);
    scheduler.addSystem(Phase::kUpdate, "move", &noop).after("damage");
    scheduler.addSystem(Phase::kUpdate, "regen", &writeHealth).after("move");  // ordered through "move"
    scheduler.addSystem(Phase::kUpdate, "ui", &readHealth).after("regen");
    EXPECT_NO_THROW(scheduler.build());
}

TEST(Scheduler, AcceptsConflictingSystemsOfDifferentPhases) {
    World world;
    Scheduler scheduler{world, RunMode::kStandalone};
    scheduler.addSystem(Phase::kFixedUpdate, "damage", &writeHealth);
    scheduler.addSystem(Phase::kUpdate, "ui", &readHealth);  // phases already run in order
    EXPECT_NO_THROW(scheduler.build());
}

TEST(Scheduler, ChecksAmbiguitiesWhateverTheRunMode) {
    World world;
    Scheduler scheduler{world, RunMode::kServer};
    scheduler.addSystem(Phase::kUpdate, "server", &writeHealth);
    scheduler.addSystem(Phase::kUpdate, "client", &writeHealth).setSide(Side::kClient);  // never run on a server
    expectBuildError(scheduler, {"'client' and 'server'"});  // they do run together in standalone
}
