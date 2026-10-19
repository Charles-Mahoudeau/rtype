/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptCallableTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <memory>
#include <span>

#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/system/DynamicQueries.hpp"
#include "rtype/ecs/system/QueryList.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/ScriptCallable.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::script_callable {
struct Health {
    int value;
};
RTYPE_ECS_COMPONENT(Health, "test.scriptCallable.Health", value);

struct Total {
    int value;
};
RTYPE_ECS_RESOURCE(Total, "test.scriptCallable.Total", value);
}  // namespace rtype::test::script_callable

namespace {
using rtype::ecs::Commands;
using rtype::ecs::DynamicQueries;
using rtype::ecs::Phase;
using rtype::ecs::QueryBuilder;
using rtype::ecs::QueryList;
using rtype::ecs::QueryMatch;
using rtype::ecs::ResMut;
using rtype::ecs::ScriptCallable;
using rtype::ecs::System;
using rtype::ecs::World;
using rtype::test::script_callable::Health;
using rtype::test::script_callable::Total;

/// @brief Stands for a handle to a script function: a call count that the "script" bumps, owned outside the system.
struct ScriptHandle {
    std::shared_ptr<int> calls{std::make_shared<int>(0)};  ///< How many times the script ran.
};

/// @return The int at the start of `bytes`.
int intOf(std::span<const std::byte> bytes) {
    int value = 0;
    std::memcpy(&value, bytes.data(), sizeof(int));
    return value;
}

/// @brief A world with two Health entities (1 and 2) and the Total resource.
class ScriptCallableTest : public ::testing::Test {
  protected:
    void SetUp() override {
        world.getResources().insert(Total{.value = 0});
        world.spawn(Health{.value = 1});
        world.spawn(Health{.value = 2});
    }

    /// @return The world's total.
    [[nodiscard]] int total() const { return world.getResources().require<Total>().value; }

    World world;  ///< The world under test.
};
}  // namespace

TEST_F(ScriptCallableTest, RunsItsCapturesWithItsParameters) {
    const ScriptHandle script;
    System system{world, "test.script",
                  ScriptCallable{[script](rtype::ecs::Query<const Health> healths, ResMut<Total> sum) {
                      ++*script.calls;
                      for (auto [entity, health] : healths) {
                          static_cast<void>(entity);
                          sum->value += health.value;
                      }
                  }}};
    system.run();
    system.run();
    EXPECT_EQ(*script.calls, 2);
    EXPECT_EQ(total(), 6);
}

TEST_F(ScriptCallableTest, DeclaresTheAccessOfItsParameters) {
    const System system{world, "test.script",
                        ScriptCallable{[](rtype::ecs::Query<const Health> /*healths*/, ResMut<Total> /*sum*/) {}}};
    ASSERT_EQ(system.getAccess().getComponents().size(), 1U);
    EXPECT_FALSE(system.getAccess().getComponents().front().write);
    ASSERT_EQ(system.getAccess().getResources().size(), 1U);
    EXPECT_TRUE(system.getAccess().getResources().front().write);
}

TEST_F(ScriptCallableTest, TakesConfiguredParameters) {
    const rtype::ecs::ComponentId health = world.getComponents().registerType<Health>();
    System system{world, "test.script", ScriptCallable{[](DynamicQueries queries, ResMut<Total> sum) {
                      for (const QueryMatch& match : queries.iter(0)) {
                          sum->value += intOf(match.get(0));
                      }
                  }},
                  QueryList{{QueryBuilder{}.read(health)}}};
    system.run();
    EXPECT_EQ(total(), 3);
}

TEST_F(ScriptCallableTest, KeepsAMutableCallableBetweenRuns) {
    System system{world, "test.script", ScriptCallable{[calls = 0](ResMut<Total> sum) mutable {
                      ++calls;
                      sum->value = calls;
                  }}};
    system.run();
    system.run();
    system.run();
    EXPECT_EQ(total(), 3);  // the callable is kept by the system, not copied for each run
}

TEST_F(ScriptCallableTest, IsAddedThroughTheApp) {
    rtype::ecs::App app;
    app.insertResource(Total{.value = 0});
    auto owned = std::make_unique<int>(40);  // move-only: the callable must be moved all the way into the system
    app.addSystem(Phase::kUpdate, "test.script",
                  ScriptCallable{[owned = std::move(owned)](ResMut<Total> sum) { sum->value = *owned + 2; }})
        .setSide(rtype::ecs::Side::kBoth);
    app.runFrame();
    EXPECT_EQ(app.getWorld().getResources().require<Total>().value, 42);
}

TEST_F(ScriptCallableTest, RecordsCommandsFlushedAfterItsPhase) {
    rtype::ecs::Schedules schedules{world, rtype::ecs::RunMode::kStandalone};
    schedules.addSystem(Phase::kUpdate, "test.spawner", ScriptCallable{[](Commands& commands) {
                            static_cast<void>(commands.spawn().add(Health{.value = 7}));
                        }});
    const std::size_t before = world.getEntityCount();
    schedules.run(Phase::kUpdate);
    EXPECT_EQ(world.getEntityCount(), before + 1);  // spawned when the phase's commands were applied
}
