/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/exceptions/MissingResourceException.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::system {
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "test.system.Position", x, y);

struct Velocity {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Velocity, "test.system.Velocity", x, y);

struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.system.Health", value);

struct Time {
    float delta;
};
RTYPE_ECS_RESOURCE(Time, "test.system.Time", delta);

/// @brief What the test systems report back, since a system keeps no state of its own.
struct Report {
    int seen;
};
RTYPE_ECS_RESOURCE(Report, "test.system.Report", seen);

struct Renderer {
    int frames;
};
RTYPE_ECS_RESOURCE(Renderer, "test.system.Renderer", frames);

struct Collision {
    std::uint32_t a;
};
RTYPE_ECS_EVENT(Collision, "test.system.Collision", a);
}  // namespace rtype::test::system

namespace {
using rtype::ecs::Changed;
using rtype::ecs::Commands;
using rtype::ecs::Entity;
using rtype::ecs::EventReader;
using rtype::ecs::EventWriter;
using rtype::ecs::NonSendMut;
using rtype::ecs::Query;
using rtype::ecs::Res;
using rtype::ecs::ResMut;
using rtype::ecs::Side;
using rtype::ecs::System;
using rtype::ecs::TypeAccess;
using rtype::ecs::World;
using rtype::test::system::Collision;
using rtype::test::system::Health;
using rtype::test::system::Position;
using rtype::test::system::Renderer;
using rtype::test::system::Report;
using rtype::test::system::Time;
using rtype::test::system::Velocity;

void movement(Query<Position, const Velocity> query, Res<Time> time) {
    for (auto [entity, position, velocity] : query) {
        position->x += velocity.x * time->delta;
        position->y += velocity.y * time->delta;
    }
}

/// @brief Counts the healths that changed since the system last ran.
void countChanged(Query<const Health, Changed<Health>> query, ResMut<Report> report) {
    report->seen = 0;
    for (auto [entity, health] : query) {
        ++report->seen;
    }
}

/// @brief Writes every health.
void heal(Query<Health> query) {
    for (auto [entity, health] : query) {
        ++health->value;
    }
}

void spawnShip(Commands& commands) { static_cast<void>(commands.spawn().add(Position{})); }

void detect(EventWriter<Collision> collisions) { collisions.send(Collision{.a = 1}); }

void react(EventReader<Collision> collisions, ResMut<Report> report) {
    report->seen += static_cast<int>(collisions.size());
}

void everything(Query<Position, const Velocity> /*query*/, Res<Time> /*time*/, ResMut<Report> /*report*/,
                NonSendMut<Renderer> /*renderer*/, Commands& /*commands*/, EventReader<Collision> /*read*/,
                EventWriter<Collision> /*write*/) {}

/// @return The accesses as a vector, to compare them.
std::vector<TypeAccess> toVector(std::span<const TypeAccess> accesses) { return {accesses.begin(), accesses.end()}; }
}  // namespace

TEST(System, RunsAFunctionWithItsParameters) {
    World world;
    world.getResources().insert(Time{.delta = 0.5F});
    const Entity ship = world.spawn(Position{.x = 0.F, .y = 0.F}, Velocity{.x = 2.F, .y = 4.F});
    System system{world, "test.movement", &movement};
    system.run();
    EXPECT_EQ(world.getComponents().get<Position>(ship)->x, 1.F);
    EXPECT_EQ(world.getComponents().get<Position>(ship)->y, 2.F);
}

TEST(System, AcceptsALambdaWithoutCaptures) {
    World world;
    const Entity ship = world.spawn(Health{.value = 1});
    System system{world, "test.lambda", [](Query<Health> query) {
                      for (auto [entity, health] : query) {
                          health->value = 9;
                      }
                  }};
    system.run();
    EXPECT_EQ(world.getComponents().get<Health>(ship)->value, 9);
}

TEST(System, HasANameAndASide) {
    World world;
    System system{world, "test.heal", &heal};
    EXPECT_EQ(system.getName(), "test.heal");
    EXPECT_EQ(system.getSide(), Side::kServer);  // the default
    system.setSide(Side::kBoth);
    EXPECT_EQ(system.getSide(), Side::kBoth);
}

TEST(System, ListsWhatItReadsAndWrites) {
    World world;
    const System system{world, "test.everything", &everything};
    const auto id = [&](const char* name) { return *world.getComponents().find(name); };
    const auto resource = [&](const char* name) { return *world.getResources().find(name); };
    EXPECT_EQ(toVector(system.getAccess().getComponents()),
              (std::vector<TypeAccess>{{.id = id("test.system.Position"), .write = true},
                                       {.id = id("test.system.Velocity"), .write = false}}));
    EXPECT_EQ(toVector(system.getAccess().getResources()),
              (std::vector<TypeAccess>{{.id = resource("test.system.Time"), .write = false},
                                       {.id = resource("test.system.Report"), .write = true},
                                       {.id = resource("test.system.Renderer"), .write = true}}));
    EXPECT_EQ(toVector(system.getAccess().getEvents()),
              (std::vector<TypeAccess>{{.id = *world.getEvents().find("test.system.Collision"), .write = true}}));
    EXPECT_TRUE(system.getAccess().isMainThread());  // NonSendMut
}

TEST(System, TheFirstRunSeesEverythingAsChanged) {
    World world;
    world.getResources().insert(Report{.seen = 0});
    world.spawn(Health{.value = 1});  // at tick 0, before the system exists
    world.spawn(Health{.value = 2});
    System counter{world, "test.count", &countChanged};
    counter.run();
    EXPECT_EQ(world.getResources().get<Report>()->seen, 2);
    counter.run();  // nothing changed since
    EXPECT_EQ(world.getResources().get<Report>()->seen, 0);
}

TEST(System, SeesWhatOtherSystemsChangedButNotItsOwnChanges) {
    World world;
    world.getResources().insert(Report{.seen = 0});
    world.spawn(Health{.value = 1});
    System counter{world, "test.count", &countChanged};
    System healer{world, "test.heal", &heal};
    counter.run();  // first run: sees the health

    healer.run();  // another system writes it
    counter.run();
    EXPECT_EQ(world.getResources().get<Report>()->seen, 1);

    System selfHealer{world, "test.self", [](Query<Health, Changed<Health>> query, ResMut<Report> report) {
                          report->seen = 0;
                          for (auto [entity, health] : query) {
                              ++health->value;  // writes what it reports
                              ++report->seen;
                          }
                      }};
    selfHealer.run();  // first run: sees the change made by `healer`
    selfHealer.run();  // its own writes are not reported to it
    EXPECT_EQ(world.getResources().get<Report>()->seen, 0);
}

TEST(System, AppliesItsCommandsOnlyWhenFlushed) {
    World world;
    System system{world, "test.spawn", &spawnShip};
    system.run();
    EXPECT_EQ(world.getEntityCount(), 0U);  // recorded, not applied
    system.flushCommands();
    EXPECT_EQ(world.getEntityCount(), 1U);
}

TEST(System, ReadsEachEventOnceAcrossRuns) {
    World world;
    world.getResources().insert(Report{.seen = 0});
    System writer{world, "test.detect", &detect};
    System reader{world, "test.react", &react};
    writer.run();
    reader.run();
    reader.run();  // the cursor kept by the system moved on
    EXPECT_EQ(world.getResources().get<Report>()->seen, 1);
}

TEST(System, AMissingResourceThrowsAndTheRunDoesNotCount) {
    World world;
    System system{world, "test.movement", &movement};
    const rtype::ecs::Tick lastRun = system.getLastRun();
    EXPECT_THROW(system.run(), rtype::ecs::exceptions::MissingResourceException);
    EXPECT_EQ(system.getLastRun(), lastRun);
}

TEST(System, CanBeMoved) {
    World world;
    const Entity ship = world.spawn(Health{.value = 1});
    System source{world, "test.heal", &heal};
    System system{std::move(source)};
    system.run();
    EXPECT_EQ(world.getComponents().get<Health>(ship)->value, 2);
    EXPECT_EQ(system.getName(), "test.heal");
}
