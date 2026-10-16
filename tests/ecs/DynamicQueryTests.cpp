/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicQueryTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <random>
#include <span>
#include <vector>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryBuilder.hpp"
#include "rtype/ecs/query/QueryMatch.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::dynamic_query {
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "test.dynamic_query.Position", x, y);

struct Velocity {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Velocity, "test.dynamic_query.Velocity", x, y);

struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.dynamic_query.Health", value);

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "test.dynamic_query.Dead");

struct Late {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Late, "test.dynamic_query.Late", value);
}  // namespace rtype::test::dynamic_query

namespace {
using rtype::ecs::ComponentId;
using rtype::ecs::DynamicQuery;
using rtype::ecs::Entity;
using rtype::ecs::QueryBuilder;
using rtype::ecs::QueryMatch;
using rtype::ecs::Result;
using rtype::ecs::Tick;
using rtype::ecs::World;
using rtype::test::dynamic_query::Dead;
using rtype::test::dynamic_query::Health;
using rtype::test::dynamic_query::Position;
using rtype::test::dynamic_query::Velocity;

/// @brief A world with four registered components, and their ids.
class DynamicQueryTest : public ::testing::Test {
  protected:
    World world;
    rtype::ecs::Components& components{world.getComponents()};
    ComponentId position{components.registerType<Position>()};
    ComponentId velocity{components.registerType<Velocity>()};
    ComponentId health{components.registerType<Health>()};
    ComponentId dead{components.registerType<Dead>()};
};

/// @return The entities a query matches, in iteration order.
std::vector<Entity> matchesOf(const DynamicQuery& query, Tick lastRun = 0) {
    std::vector<Entity> entities;
    for (const QueryMatch& match : query.iter(lastRun)) {
        entities.push_back(match.getEntity());
    }
    return entities;
}

/// @return The entities sorted by index, to compare sets.
std::vector<Entity> sorted(std::vector<Entity> entities) {
    std::ranges::sort(entities, {}, &Entity::getBits);
    return entities;
}

/// @return A copy of the value held in `bytes`.
template <typename T>
T valueOf(std::span<const std::byte> bytes) {
    T value;
    std::memcpy(&value, bytes.data(), sizeof(T));
    return value;
}
}  // namespace

TEST_F(DynamicQueryTest, MatchesEntitiesWithEveryRequiredComponent) {
    const Entity both = world.spawn(Position{}, Velocity{});
    world.spawn(Position{});
    world.spawn(Velocity{});
    const Result<DynamicQuery> query = QueryBuilder{}.read(position).read(velocity).build(components);
    ASSERT_TRUE(query);
    EXPECT_EQ(matchesOf(*query), std::vector<Entity>{both});
}

TEST_F(DynamicQueryTest, SkipsExcludedEntities) {
    const Entity alive = world.spawn(Position{});
    world.spawn(Position{}, Dead{});
    const Result<DynamicQuery> query = QueryBuilder{}.read(position).without(dead).build(components);
    ASSERT_TRUE(query);
    EXPECT_EQ(matchesOf(*query), std::vector<Entity>{alive});
}

TEST_F(DynamicQueryTest, MatchesATagWithWith) {
    const Entity corpse = world.spawn(Position{}, Dead{});
    world.spawn(Position{});
    const Result<DynamicQuery> query = QueryBuilder{}.with(dead).read(position).build(components);
    ASSERT_TRUE(query);
    EXPECT_EQ(matchesOf(*query), std::vector<Entity>{corpse});
}

TEST_F(DynamicQueryTest, YieldsNothingFromAnEmptyPool) {
    world.spawn(Position{});
    const Result<DynamicQuery> query = QueryBuilder{}.read(position).read(health).build(components);
    ASSERT_TRUE(query);
    EXPECT_TRUE(matchesOf(*query).empty());
}

TEST_F(DynamicQueryTest, ReadsValuesByDataTerm) {
    const Entity ship = world.spawn(Position{.x = 1.F, .y = 2.F}, Health{.value = 7});
    const Result<DynamicQuery> query = QueryBuilder{}.with(position).read(health).build(components);
    ASSERT_TRUE(query);
    int count = 0;
    for (const QueryMatch& match : query->iter(0)) {
        EXPECT_EQ(match.getEntity(), ship);
        EXPECT_EQ(valueOf<Health>(match.get(0)).value, 7);  // with is not a data term: data term 0 is health
        EXPECT_TRUE(match.get(1).empty());                  // no such data term
        ++count;
    }
    EXPECT_EQ(count, 1);
}

TEST_F(DynamicQueryTest, YieldsAnEmptyValueForAnAbsentOptional) {
    const Entity with = world.spawn(Position{}, Health{.value = 3});
    const Entity without = world.spawn(Position{});
    const Result<DynamicQuery> query = QueryBuilder{}.read(position).optionalRead(health).build(components);
    ASSERT_TRUE(query);
    for (const QueryMatch& match : query->iter(0)) {
        if (match.getEntity() == with) {
            EXPECT_EQ(valueOf<Health>(match.get(1)).value, 3);
        } else {
            EXPECT_EQ(match.getEntity(), without);
            EXPECT_TRUE(match.get(1).empty());
        }
    }
    EXPECT_EQ(sorted(matchesOf(*query)), sorted({with, without}));
}

TEST_F(DynamicQueryTest, WritesThroughGetMutAndStampsTheTick) {
    const Entity ship = world.spawn(Health{.value = 1});
    world.advanceChangeTick(5);
    const Result<DynamicQuery> query = QueryBuilder{}.write(health).build(components);
    ASSERT_TRUE(query);
    for (QueryMatch& match : query->iter(0)) {
        const std::span<std::byte> bytes = match.getMut(0);
        const Health updated{.value = 42};
        std::memcpy(bytes.data(), &updated, sizeof(Health));
    }
    EXPECT_EQ(components.get<Health>(ship)->value, 42);
    const std::optional<rtype::ecs::ComponentTicks> ticks = components.getTicks<Health>(ship);
    ASSERT_TRUE(ticks);
    EXPECT_EQ(ticks->added, 0U);
    EXPECT_EQ(ticks->changed, 5U);
}

TEST_F(DynamicQueryTest, ReadingNeverStampsTheTick) {
    const Entity ship = world.spawn(Health{.value = 1});
    world.advanceChangeTick(5);
    const Result<DynamicQuery> query = QueryBuilder{}.write(health).build(components);
    ASSERT_TRUE(query);
    for (const QueryMatch& match : query->iter(0)) {
        EXPECT_EQ(valueOf<Health>(match.get(0)).value, 1);
    }
    EXPECT_EQ(components.getTicks<Health>(ship)->changed, 0U);
}

TEST_F(DynamicQueryTest, RefusesWriteAccessToAReadTerm) {
    world.spawn(Health{.value = 1});
    const Result<DynamicQuery> query = QueryBuilder{}.read(health).build(components);
    ASSERT_TRUE(query);
    for (QueryMatch& match : query->iter(0)) {
        EXPECT_TRUE(match.getMut(0).empty());
        EXPECT_TRUE(match.getMut(3).empty());
    }
}

TEST_F(DynamicQueryTest, FiltersOnAdded) {
    world.spawn(Position{});
    world.advanceChangeTick();
    const Tick lastRun = world.getChangeTick();
    world.advanceChangeTick();
    const Entity recent = world.spawn(Position{});
    const Result<DynamicQuery> query = QueryBuilder{}.added(position).build(components);
    ASSERT_TRUE(query);
    EXPECT_EQ(matchesOf(*query, lastRun), std::vector<Entity>{recent});
}

TEST_F(DynamicQueryTest, FiltersOnChanged) {
    const Entity first = world.spawn(Health{.value = 1});
    world.spawn(Health{.value = 2});
    world.advanceChangeTick();
    const Tick lastRun = world.getChangeTick();
    world.advanceChangeTick();
    components.getMut<Health>(first)->value = 10;
    const Result<DynamicQuery> query = QueryBuilder{}.read(health).changed(health).build(components);
    ASSERT_TRUE(query);
    EXPECT_EQ(matchesOf(*query, lastRun), std::vector<Entity>{first});
}

TEST_F(DynamicQueryTest, DrivesFromTheSmallestRequiredPool) {
    for (int count = 0; count < 5; ++count) {
        world.spawn(Position{}, Velocity{});
    }
    world.spawn(Position{}, Velocity{}, Health{});
    const Result<DynamicQuery> query =
        QueryBuilder{}.read(position).optionalRead(dead).read(velocity).with(health).build(components);
    ASSERT_TRUE(query);
    std::array<rtype::ecs::storage::SparseSetStorage*, 4> pools{};
    query->resolvePools(pools);
    EXPECT_EQ(query->findDriver(pools), 3U);  // the health pool (1 entity); the empty dead pool is optional
}

TEST_F(DynamicQueryTest, SeesPoolsAfterTheWorldRegistersMore) {
    const Entity ship = world.spawn(Position{});
    const Result<DynamicQuery> query = QueryBuilder{}.read(position).build(components);
    ASSERT_TRUE(query);
    components.registerType<rtype::test::dynamic_query::Late>();  // may move every pool
    components.add(ship, rtype::test::dynamic_query::Late{.value = 1});
    EXPECT_EQ(matchesOf(*query), std::vector<Entity>{ship});
}

TEST_F(DynamicQueryTest, EqualsABruteForceReferenceOnRandomWorlds) {
    constexpr int kEntityCount = 300;
    constexpr int kQueryCount = 400;
    const std::array<ComponentId, 4> ids{position, velocity, health, dead};
    std::mt19937 random{1234};  // fixed seed: a failure reproduces
    std::bernoulli_distribution half{0.5};

    std::vector<Entity> entities;
    for (int index = 0; index < kEntityCount; ++index) {
        const Entity entity = world.spawn();
        entities.push_back(entity);
        if (half(random)) {
            components.add(entity, Position{});
        }
        if (half(random)) {
            components.add(entity, Velocity{});
        }
        if (half(random)) {
            components.add(entity, Health{});
        }
        if (half(random)) {
            components.add(entity, Dead{});
        }
        if (index % 3 == 0) {
            world.advanceChangeTick();
        }
    }
    for (const Entity entity : entities) {  // writes at later ticks, and despawns, scramble ticks and dense orders
        if (half(random)) {
            static_cast<void>(components.getMut(entity, health));  // marks it changed now
        }
        if (std::uniform_int_distribution{0, 9}(random) == 0) {
            world.despawn(entity);
        }
        world.advanceChangeTick();
    }
    const Tick thisRun = world.getChangeTick();

    constexpr std::array<std::optional<rtype::ecs::TermKind>, 9> kKinds{std::nullopt,
                                                                        rtype::ecs::TermKind::kRead,
                                                                        rtype::ecs::TermKind::kWrite,
                                                                        rtype::ecs::TermKind::kOptionalRead,
                                                                        rtype::ecs::TermKind::kOptionalWrite,
                                                                        rtype::ecs::TermKind::kWith,
                                                                        rtype::ecs::TermKind::kWithout,
                                                                        rtype::ecs::TermKind::kAdded,
                                                                        rtype::ecs::TermKind::kChanged};
    int built = 0;
    int nonEmpty = 0;
    for (int queryIndex = 0; queryIndex < kQueryCount; ++queryIndex) {
        std::vector<rtype::ecs::QueryTerm> terms;
        for (const ComponentId id : ids) {
            const std::optional<rtype::ecs::TermKind> kind =
                *std::next(kKinds.begin(), std::uniform_int_distribution{0, 8}(random));
            if (kind) {
                terms.push_back({.id = id, .kind = *kind});
            }
        }
        const Result<DynamicQuery> query = DynamicQuery::create(components, terms);
        if (!query) {
            continue;  // invalid combinations are covered by the builder tests
        }
        ++built;
        const Tick lastRun = std::uniform_int_distribution<Tick>{0, thisRun}(random);

        std::vector<Entity> expected;
        for (const Entity entity : entities) {
            if (!world.isAlive(entity)) {
                continue;
            }
            const bool matches = std::ranges::all_of(terms, [&](const rtype::ecs::QueryTerm& term) {
                const std::optional<rtype::ecs::ComponentTicks> ticks = components.getTicks(entity, term.id);
                switch (term.kind) {
                    case rtype::ecs::TermKind::kWithout:
                        return !ticks;
                    case rtype::ecs::TermKind::kOptionalRead:
                    case rtype::ecs::TermKind::kOptionalWrite:
                        return true;
                    case rtype::ecs::TermKind::kAdded:
                        return ticks && ticks->isAdded(lastRun, thisRun);
                    case rtype::ecs::TermKind::kChanged:
                        return ticks && ticks->isChanged(lastRun, thisRun);
                    default:
                        return static_cast<bool>(ticks);
                }
            });
            if (matches) {
                expected.push_back(entity);
            }
        }
        EXPECT_EQ(sorted(matchesOf(*query, lastRun)), sorted(expected)) << "query " << queryIndex;
        nonEmpty += expected.empty() ? 0 : 1;
    }
    EXPECT_GT(built, kQueryCount / 4);  // enough valid queries to mean something
    EXPECT_GT(nonEmpty, built / 3);     // and many of them match entities
}
