/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** QueryTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <random>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <vector>

#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/query/QueryIteration.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/query/QueryState.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::query {
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "test.query.Position", x, y);

struct Velocity {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Velocity, "test.query.Velocity", x, y);

struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.query.Health", value);

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "test.query.Dead");
}  // namespace rtype::test::query

namespace {
using rtype::ecs::Added;
using rtype::ecs::Changed;
using rtype::ecs::Entity;
using rtype::ecs::Mut;
using rtype::ecs::Optional;
using rtype::ecs::QueryState;
using rtype::ecs::Tick;
using rtype::ecs::With;
using rtype::ecs::Without;
using rtype::ecs::World;
using rtype::test::query::Dead;
using rtype::test::query::Health;
using rtype::test::query::Position;
using rtype::test::query::Velocity;

// A match is the entity, then one item per term that has one: filters are left out.
static_assert(std::is_same_v<QueryState<Position, const Velocity, With<Dead>>::Item,
                             std::tuple<Entity, Mut<Position>, const Velocity&>>);
static_assert(
    std::is_same_v<QueryState<const Position, Without<Dead>, Optional<const Health>, Optional<Velocity>>::Item,
                   std::tuple<Entity, const Position&, const Health*, Mut<Velocity>>>);
static_assert(std::is_same_v<QueryState<With<Dead>>::Item, std::tuple<Entity>>);

/// @return The entities a typed query matches, sorted to compare sets.
template <typename... Terms>
std::vector<Entity> matchesOf(const QueryState<Terms...>& query, Tick lastRun = 0) {
    std::vector<Entity> entities;
    for (auto match : query.iter(lastRun)) {
        entities.push_back(std::get<Entity>(match));
    }
    std::ranges::sort(entities, {}, &Entity::getBits);
    return entities;
}

/// @return The entities a dynamic query matches, sorted to compare sets.
std::vector<Entity> matchesOf(const rtype::ecs::DynamicQuery& query, Tick lastRun) {
    std::vector<Entity> entities;
    for (const rtype::ecs::QueryMatch& match : query.iter(lastRun)) {
        entities.push_back(match.getEntity());
    }
    std::ranges::sort(entities, {}, &Entity::getBits);
    return entities;
}
}  // namespace

TEST(QueryState, MovesEntitiesWithStructuredBindings) {
    World world;
    const Entity ship = world.spawn(Position{.x = 0.F, .y = 0.F}, Velocity{.x = 1.F, .y = 2.F});
    const Entity rock = world.spawn(Position{.x = 5.F, .y = 5.F});
    const QueryState<Position, const Velocity> movement{world.getComponents()};

    for (auto [entity, position, velocity] : movement.iter(0)) {
        EXPECT_EQ(entity, ship);
        position->x += velocity.x;
        position->y += velocity.y;
    }
    EXPECT_EQ(world.getComponents().get<Position>(ship)->x, 1.F);
    EXPECT_EQ(world.getComponents().get<Position>(ship)->y, 2.F);
    EXPECT_EQ(world.getComponents().get<Position>(rock)->x, 5.F);  // no velocity: not matched
}

TEST(QueryState, IteratingWithoutWritingMarksNothingChanged) {
    World world;
    const Entity first = world.spawn(Health{.value = 1});
    const Entity second = world.spawn(Health{.value = 2});
    world.advanceChangeTick(3);
    const QueryState<Health> query{world.getComponents()};

    for (auto [entity, health] : query.iter(0)) {
        if (health.get().value == 2) {  // reading through get() does not stamp
            health->value = 20;         // writing through -> does
        }
    }
    EXPECT_EQ(world.getComponents().getTicks<Health>(first)->changed, 0U);
    EXPECT_EQ(world.getComponents().getTicks<Health>(second)->changed, 3U);
    EXPECT_EQ(world.getComponents().get<Health>(second)->value, 20);
}

TEST(QueryState, YieldsOptionalItemsThatMayBeEmpty) {
    World world;
    const Entity full = world.spawn(Position{}, Health{.value = 7}, Velocity{.x = 1.F, .y = 0.F});
    const Entity bare = world.spawn(Position{});
    const QueryState<const Position, Optional<const Health>, Optional<Velocity>> query{world.getComponents()};

    int count = 0;
    for (auto [entity, position, health, velocity] : query.iter(0)) {
        if (entity == full) {
            ASSERT_NE(health, nullptr);
            EXPECT_EQ(health->value, 7);
            ASSERT_TRUE(velocity);
            velocity->x = 9.F;
        } else {
            EXPECT_EQ(entity, bare);
            EXPECT_EQ(health, nullptr);
            EXPECT_FALSE(velocity);
        }
        ++count;
    }
    EXPECT_EQ(count, 2);
    EXPECT_EQ(world.getComponents().get<Velocity>(full)->x, 9.F);
}

TEST(QueryState, FiltersWithAndWithout) {
    World world;
    const Entity corpse = world.spawn(Position{}, Dead{});
    const Entity alive = world.spawn(Position{});
    EXPECT_EQ(matchesOf(QueryState<const Position, With<Dead>>{world.getComponents()}), std::vector<Entity>{corpse});
    EXPECT_EQ(matchesOf(QueryState<const Position, Without<Dead>>{world.getComponents()}), std::vector<Entity>{alive});
}

TEST(QueryState, FiltersAddedAndChanged) {
    World world;
    const Entity old = world.spawn(Health{.value = 1});
    world.advanceChangeTick();
    const Tick lastRun = world.getChangeTick();
    world.advanceChangeTick();
    const Entity recent = world.spawn(Health{.value = 2});
    world.getComponents().getMut<Health>(old)->value = 10;

    EXPECT_EQ(matchesOf(QueryState<Added<Health>>{world.getComponents()}, lastRun), std::vector<Entity>{recent});
    std::vector<Entity> both{old, recent};
    std::ranges::sort(both, {}, &Entity::getBits);
    EXPECT_EQ(matchesOf(QueryState<const Health, Changed<Health>>{world.getComponents()}, lastRun), both);
}

TEST(QueryState, RegistersItsComponents) {
    World world;
    const QueryState<Position, With<Dead>> query{world.getComponents()};
    EXPECT_TRUE(world.getComponents().getRegistry().find<Position>() != nullptr);
    EXPECT_TRUE(world.getComponents().getRegistry().find<Dead>() != nullptr);

    const Entity later = world.spawn(Position{}, Dead{});  // the query sees entities spawned after it was built
    EXPECT_EQ(matchesOf(query), std::vector<Entity>{later});
}

TEST(QueryState, KeepsTheOptionsOfAnAlreadyRegisteredComponent) {
    World world;
    world.getComponents().registerType<Position>(
        rtype::ecs::ComponentOptions{.replication = rtype::ecs::ReplicationMode::kEveryChange});
    EXPECT_NO_THROW(QueryState<const Position>{world.getComponents()});
    EXPECT_EQ(world.getComponents().getRegistry().find<Position>()->type.replication,
              rtype::ecs::ReplicationMode::kEveryChange);
}

TEST(QueryState, CompilesDownToTheDynamicQuery) {
    World world;
    const QueryState<Position, const Velocity, Without<Dead>> query{world.getComponents()};
    const rtype::ecs::ComponentId position = world.getComponents().getRegistry().find<Position>()->type.id;
    const rtype::ecs::ComponentId velocity = world.getComponents().getRegistry().find<Velocity>()->type.id;
    const rtype::ecs::ComponentId dead = world.getComponents().getRegistry().find<Dead>()->type.id;

    const std::vector<rtype::ecs::QueryTerm> expected{{.id = position, .kind = rtype::ecs::TermKind::kWrite},
                                                      {.id = velocity, .kind = rtype::ecs::TermKind::kRead},
                                                      {.id = dead, .kind = rtype::ecs::TermKind::kWithout}};
    const std::span<const rtype::ecs::QueryTerm> terms = query.getDynamic().getTerms();
    EXPECT_EQ(std::vector<rtype::ecs::QueryTerm>(terms.begin(), terms.end()), expected);
    const std::vector<rtype::ecs::ComponentAccess> access{{.id = position, .write = true},
                                                          {.id = velocity, .write = false}};
    EXPECT_EQ(std::vector<rtype::ecs::ComponentAccess>(query.getAccess().begin(), query.getAccess().end()), access);
}

TEST(QueryState, MatchesTheSameEntitiesAsTheDynamicQuery) {
    World world;
    rtype::ecs::Components& components = world.getComponents();
    std::mt19937 random{99};
    std::bernoulli_distribution half{0.5};
    for (int index = 0; index < 200; ++index) {
        const Entity entity = world.spawn();
        if (half(random)) {
            components.add(entity, Position{});
        }
        if (half(random)) {
            components.add(entity, Velocity{});
        }
        if (half(random)) {
            components.add(entity, Dead{});
        }
    }
    const QueryState<Position, Optional<const Velocity>, Without<Dead>> typed{components};
    const rtype::ecs::Result<rtype::ecs::DynamicQuery> dynamic =
        rtype::ecs::QueryBuilder{}
            .write(*components.find("test.query.Position"))
            .optionalRead(*components.find("test.query.Velocity"))
            .without(*components.find("test.query.Dead"))
            .build(components);
    ASSERT_TRUE(dynamic);
    const std::vector<Entity> expected = matchesOf(*dynamic, 0);
    EXPECT_FALSE(expected.empty());
    EXPECT_EQ(matchesOf(typed), expected);
}

// Both passes are C++20 input ranges: std::ranges algorithms and views work on them.
static_assert(std::input_iterator<rtype::ecs::QueryIteration::Iterator>);
static_assert(std::sentinel_for<std::default_sentinel_t, rtype::ecs::QueryIteration::Iterator>);
static_assert(std::ranges::input_range<rtype::ecs::QueryIteration>);
static_assert(std::input_iterator<QueryState<Position, const Velocity>::Iteration::Iterator>);
static_assert(std::ranges::input_range<QueryState<Position, const Velocity>::Iteration>);

TEST(QueryState, WorksWithRangesAlgorithms) {
    World world;
    world.spawn(Health{.value = 5});
    world.spawn(Health{.value = 50});
    world.spawn(Health{.value = 500});
    const QueryState<const Health> typed{world.getComponents()};
    EXPECT_EQ(std::ranges::distance(typed.iter(0)), 3);
    EXPECT_EQ(std::ranges::count_if(typed.iter(0), [](const auto& match) { return std::get<1>(match).value > 10; }), 2);

    const rtype::ecs::Result<rtype::ecs::DynamicQuery> dynamic =
        rtype::ecs::QueryBuilder{}.read(*world.getComponents().find("test.query.Health")).build(world.getComponents());
    ASSERT_TRUE(dynamic);
    EXPECT_EQ(std::ranges::distance(dynamic->iter(0)), 3);
}
