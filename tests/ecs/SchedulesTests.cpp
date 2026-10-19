/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SchedulesTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/exceptions/ScheduleException.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/FrameClock.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::schedules {
struct Runs {
    int count;
};
RTYPE_ECS_RESOURCE(Runs, "test.schedules.Runs", count);

struct Health {
    int value;
};
RTYPE_ECS_COMPONENT(Health, "test.schedules.Health", value);
}  // namespace rtype::test::schedules

namespace {
using rtype::ecs::FrameClock;
using rtype::ecs::Phase;
using rtype::ecs::Query;
using rtype::ecs::ResMut;
using rtype::ecs::RunMode;
using rtype::ecs::Schedules;
using rtype::ecs::World;
using rtype::ecs::exceptions::ScheduleException;
using rtype::test::schedules::Health;
using rtype::test::schedules::Runs;

void noop() {}
void count(ResMut<Runs> runs) { ++runs->count; }
void writeHealth(Query<Health> /*healths*/) {}
void readHealth(Query<const Health> /*healths*/) {}

/// @brief A clock that never runs a fixed tick.
class NoTicks : public FrameClock {
  public:
    NoTicks() = default;
    ~NoTicks() override = default;
    NoTicks(const NoTicks& other) = delete;
    NoTicks& operator=(const NoTicks& other) = delete;
    NoTicks(NoTicks&& other) = delete;
    NoTicks& operator=(NoTicks&& other) = delete;

    std::uint32_t startFrame(World& /*world*/) override { return 0; }
    void finishFixedTick(World& /*world*/) override {}
    void startUpdate(World& /*world*/) override {}
};

/// @return The order of a phase, as strings, to compare it.
std::vector<std::string> orderOf(const Schedules& schedules, Phase phase) {
    std::vector<std::string> names;
    for (const std::string_view name : schedules.getOrder(phase)) {
        names.emplace_back(name);
    }
    return names;
}

/// @brief Expects `call` to throw a ScheduleException whose message contains `text`.
template <typename Call>
void expectScheduleError(Call call, std::string_view text) {
    try {
        call();
        FAIL() << "no ScheduleException";
    } catch (const ScheduleException& error) {
        EXPECT_NE(std::string_view{error.what()}.find(text), std::string_view::npos) << error.what();
    }
}
}  // namespace

TEST(Schedules, HoldsOneSchedulePerPhase) {
    World world;
    const Schedules schedules{world, RunMode::kServer};
    EXPECT_EQ(schedules.getRunMode(), RunMode::kServer);
    for (const Phase phase : {Phase::kStartup, Phase::kPreUpdate, Phase::kFixedUpdate, Phase::kUpdate,
                              Phase::kPostUpdate, Phase::kRender}) {
        EXPECT_EQ(schedules.getSchedule(phase).getPhase(), phase);
        EXPECT_EQ(schedules.getSchedule(phase).getRunMode(), RunMode::kServer);
    }
}

TEST(Schedules, AddsSystemsToTheirPhase) {
    World world;
    Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kUpdate, "update", &noop);
    schedules.addSystem(Phase::kRender, "render", &noop);
    schedules.build();
    EXPECT_EQ(orderOf(schedules, Phase::kUpdate), (std::vector<std::string>{"update"}));
    EXPECT_EQ(orderOf(schedules, Phase::kRender), (std::vector<std::string>{"render"}));
    EXPECT_TRUE(schedules.getOrder(Phase::kStartup).empty());
}

TEST(Schedules, RejectsANameUsedInAnotherPhase) {
    World world;
    Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kUpdate, "rtype.movement", &noop);
    expectScheduleError([&] { schedules.addSystem(Phase::kRender, "rtype.movement", &noop); },
                        "Two systems are named 'rtype.movement' (phases Update and Render)");
    EXPECT_FALSE(schedules.getSchedule(Phase::kRender).contains("rtype.movement"));
}

TEST(Schedules, RejectsATargetInAnotherPhase) {
    World world;
    Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kPreUpdate, "rtype.input", &noop);
    schedules.addSystem(Phase::kFixedUpdate, "rtype.movement", &noop).before("rtype.input");
    expectScheduleError([&] { schedules.build(); },
                        "'rtype.movement' runs before 'rtype.input', which is not a system of phase FixedUpdate");
}

TEST(Schedules, AcceptsConflictingSystemsOfDifferentPhases) {
    World world;
    Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kFixedUpdate, "damage", &writeHealth);
    schedules.addSystem(Phase::kUpdate, "ui", &readHealth);  // phases already run in order
    EXPECT_NO_THROW(schedules.build());
}

TEST(Schedules, RunsOnlyTheGivenPhase) {
    World world;
    world.getResources().insert(Runs{.count = 0});
    Schedules schedules{world, RunMode::kStandalone};
    schedules.addSystem(Phase::kRender, "count", &count);
    schedules.run(Phase::kUpdate);
    EXPECT_EQ(world.getResources().get<Runs>()->count, 0);
    schedules.run(Phase::kRender);
    EXPECT_EQ(world.getResources().get<Runs>()->count, 1);
}

TEST(Schedules, KeepsTheClockItIsGiven) {
    World world;
    Schedules schedules{world, RunMode::kStandalone};
    EXPECT_EQ(schedules.getClock(), nullptr);
    auto clock = std::make_unique<NoTicks>();
    FrameClock* const installed = clock.get();
    schedules.setClock(std::move(clock));
    EXPECT_EQ(schedules.getClock(), installed);
    schedules.setClock(nullptr);
    EXPECT_EQ(schedules.getClock(), nullptr);
}
