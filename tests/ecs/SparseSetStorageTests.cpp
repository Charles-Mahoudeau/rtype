/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SparseSetStorageTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"

namespace {
/// @brief Number of Counter instances alive, to detect leaks and double destruction.
int liveCounters = 0;

/// @brief Counts its own instances.
struct Counter {
    Counter() noexcept { ++liveCounters; }
    ~Counter() { --liveCounters; }
    Counter(const Counter& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(const Counter& /*other*/) noexcept = default;
    Counter(Counter&& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(Counter&& /*other*/) noexcept = default;
};
}  // namespace

namespace rtype::test::pool {
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "test.pool.Position", x, y);

struct Scaled {
    float scale{1.0F};
};
RTYPE_ECS_COMPONENT(Scaled, "test.pool.Scaled", scale);

struct Tracked {
    std::vector<std::int32_t> items;
    Counter counter;
};
RTYPE_ECS_COMPONENT(Tracked, "test.pool.Tracked");

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "test.pool.Dead");
}  // namespace rtype::test::pool

namespace {
using rtype::ecs::Entity;
using rtype::ecs::storage::SparseSetStorage;
using rtype::test::pool::Position;

/// @brief A ComponentInfo that outlives the pools built from it, as the registry's do.
template <typename T>
const rtype::ecs::ComponentInfo& infoOf() {
    static const rtype::ecs::ComponentInfo kInfo = rtype::ecs::ComponentTraits<T>::makeInfo();
    return kInfo;
}

/// @brief Adds a Position with the given coordinates.
void addPosition(SparseSetStorage& pool, Entity entity, float x, rtype::ecs::Tick tick = 0) {
    *rtype::ecs::storage::viewAs<Position>(pool.emplace(entity, tick)) = Position{.x = x, .y = -x};
}

/// @return The x coordinate of the entity's Position.
float xOf(SparseSetStorage& pool, Entity entity) { return rtype::ecs::storage::viewAs<Position>(pool.get(entity))->x; }
}  // namespace

TEST(SparseSetStorage, StartsEmpty) {
    const SparseSetStorage pool{infoOf<Position>()};
    EXPECT_EQ(pool.getSize(), 0U);
    EXPECT_TRUE(pool.getEntities().empty());
    EXPECT_FALSE(pool.contains(Entity{1, 1}));
    EXPECT_FALSE(pool.find(Entity{1, 1}));
    EXPECT_TRUE(pool.get(Entity{1, 1}).empty());
    EXPECT_EQ(&pool.getInfo(), &infoOf<Position>());
}

TEST(SparseSetStorage, EmplaceStoresADefaultValue) {
    SparseSetStorage pool{infoOf<Position>()};
    const Entity entity{3, 1};
    const Position* position = rtype::ecs::storage::viewAs<Position>(pool.emplace(entity, 0));
    EXPECT_EQ(position->x, 0.0F);
    EXPECT_EQ(position->y, 0.0F);
    EXPECT_TRUE(pool.contains(entity));
    EXPECT_EQ(pool.find(entity), std::optional<std::uint32_t>{0});
    EXPECT_EQ(pool.get(entity).data(), pool.at(0).data());
}

TEST(SparseSetStorage, EmplaceAppliesDefaultMemberInitializers) {
    SparseSetStorage pool{infoOf<rtype::test::pool::Scaled>()};
    EXPECT_EQ(rtype::ecs::storage::viewAs<rtype::test::pool::Scaled>(pool.emplace(Entity{1, 1}, 0))->scale, 1.0F);
}

TEST(SparseSetStorage, KeepsEntitiesInInsertionOrder) {
    SparseSetStorage pool{infoOf<Position>()};
    const std::vector<Entity> entities{Entity{3, 1}, Entity{6, 1}, Entity{1, 2}};
    for (const Entity entity : entities) {
        addPosition(pool, entity, 0.0F);
    }
    EXPECT_EQ(std::vector<Entity>(pool.getEntities().begin(), pool.getEntities().end()), entities);
}

TEST(SparseSetStorage, EachEntityResolvesToItsOwnValueAfterGrowth) {
    SparseSetStorage pool{infoOf<Position>()};
    for (std::uint32_t index = 0; index < 200; ++index) {
        addPosition(pool, Entity{index * 7, 1}, static_cast<float>(index));
    }
    ASSERT_EQ(pool.getSize(), 200U);
    for (std::uint32_t index = 0; index < 200; ++index) {
        EXPECT_EQ(xOf(pool, Entity{index * 7, 1}), static_cast<float>(index)) << "index " << index * 7;
    }
}

TEST(SparseSetStorage, StampsBothTicksOnInsertion) {
    SparseSetStorage pool{infoOf<Position>()};
    addPosition(pool, Entity{1, 1}, 0.0F, 3);
    addPosition(pool, Entity{2, 1}, 0.0F, 7);
    EXPECT_EQ(pool.getAddedTicks()[0], 3U);
    EXPECT_EQ(pool.getAddedTicks()[1], 7U);
    EXPECT_EQ(pool.getChangedTicks()[0], 3U);
    EXPECT_EQ(pool.getChangedTicks()[1], 7U);
    pool.getChangedTicks()[1] = 9;  // what a write through Mut<T> will do
    EXPECT_EQ(std::as_const(pool).getChangedTicks()[1], 9U);
    EXPECT_EQ(std::as_const(pool).getAddedTicks()[1], 7U);
}

TEST(SparseSetStorage, DoesNotFindAStaleHandle) {
    SparseSetStorage pool{infoOf<Position>()};
    addPosition(pool, Entity{5, 1}, 4.0F);
    const Entity reused{5, 2};  // same index, newer generation
    EXPECT_FALSE(pool.contains(reused));
    EXPECT_FALSE(pool.find(reused));
    EXPECT_TRUE(pool.get(reused).empty());
    EXPECT_TRUE(pool.contains(Entity{5, 1}));
}

TEST(SparseSetStorage, FindsEntitiesWithFarApartIndices) {
    SparseSetStorage pool{infoOf<Position>()};
    addPosition(pool, Entity{0, 1}, 1.0F);
    addPosition(pool, Entity{1'000'000, 1}, 2.0F);
    EXPECT_EQ(xOf(pool, Entity{0, 1}), 1.0F);
    EXPECT_EQ(xOf(pool, Entity{1'000'000, 1}), 2.0F);
    EXPECT_FALSE(pool.contains(Entity{500'000, 1}));
    EXPECT_FALSE(pool.contains(Entity{1'000'001, 1}));
}

TEST(SparseSetStorage, StoresTagsWithoutBytes) {
    SparseSetStorage pool{infoOf<rtype::test::pool::Dead>()};
    const Entity entity{2, 1};
    EXPECT_TRUE(pool.emplace(entity, 5).empty());
    EXPECT_TRUE(pool.contains(entity));
    EXPECT_TRUE(pool.get(entity).empty());
    EXPECT_EQ(pool.getSize(), 1U);
    EXPECT_EQ(pool.getAddedTicks()[0], 5U);
}

TEST(SparseSetStorage, ReadsThroughAConstPool) {
    SparseSetStorage pool{infoOf<Position>()};
    addPosition(pool, Entity{1, 1}, 8.0F);
    const SparseSetStorage& readOnly = pool;
    EXPECT_EQ(rtype::ecs::storage::viewAs<Position>(readOnly.get(Entity{1, 1}))->x, 8.0F);
    EXPECT_EQ(rtype::ecs::storage::viewAs<Position>(readOnly.at(0))->x, 8.0F);
    EXPECT_TRUE(readOnly.get(Entity{2, 1}).empty());
}

TEST(SparseSetStorage, ReserveKeepsValues) {
    SparseSetStorage pool{infoOf<Position>()};
    addPosition(pool, Entity{1, 1}, 3.0F);
    pool.reserve(1000);
    EXPECT_EQ(xOf(pool, Entity{1, 1}), 3.0F);
    EXPECT_EQ(pool.getSize(), 1U);
}

TEST(SparseSetStorage, DestroysNonTrivialValues) {
    liveCounters = 0;
    {
        SparseSetStorage pool{infoOf<rtype::test::pool::Tracked>()};
        for (std::uint32_t index = 0; index < 20; ++index) {
            rtype::ecs::storage::viewAs<rtype::test::pool::Tracked>(pool.emplace(Entity{index, 1}, 0))->items = {1, 2};
        }
        EXPECT_EQ(liveCounters, 20);
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(SparseSetStorage, MovingTransfersTheValues) {
    SparseSetStorage pool{infoOf<Position>()};
    addPosition(pool, Entity{4, 1}, 6.0F);
    SparseSetStorage moved{std::move(pool)};
    EXPECT_EQ(xOf(moved, Entity{4, 1}), 6.0F);
    SparseSetStorage assigned{infoOf<Position>()};
    addPosition(assigned, Entity{9, 1}, 1.0F);
    assigned = std::move(moved);
    EXPECT_EQ(xOf(assigned, Entity{4, 1}), 6.0F);
    EXPECT_FALSE(assigned.contains(Entity{9, 1}));
}
