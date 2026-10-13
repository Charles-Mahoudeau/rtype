/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ByteColumnTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory_resource>
#include <utility>
#include <vector>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/storage/ByteColumn.hpp"

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

/// @brief A memory resource that records every allocation and checks it is given back with the same size and
/// alignment.
class CountingResource : public std::pmr::memory_resource {
  public:
    int allocations{0};
    int deallocations{0};
    bool mismatch{false};

  private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        ++allocations;
        void* data = std::pmr::new_delete_resource()->allocate(bytes, alignment);
        _live[data] = {bytes, alignment};
        return data;
    }

    void do_deallocate(void* data, std::size_t bytes, std::size_t alignment) override {
        ++deallocations;
        const auto found = _live.find(data);
        if (found == _live.end() || found->second != std::pair{bytes, alignment}) {
            mismatch = true;
        } else {
            _live.erase(found);
        }
        std::pmr::new_delete_resource()->deallocate(data, bytes, alignment);
    }

    [[nodiscard]] bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }

    std::map<void*, std::pair<std::size_t, std::size_t>> _live;
};
}  // namespace

namespace rtype::test {
struct Plain {
    std::int32_t a;
    float b;
};
RTYPE_ECS_COMPONENT(Plain, "test.Plain", a, b);

struct Scaled {
    float scale{1.0F};
};
RTYPE_ECS_COMPONENT(Scaled, "test.Scaled", scale);

struct Tracked {
    std::vector<std::int32_t> items;
    Counter counter;
};
RTYPE_ECS_COMPONENT(Tracked, "test.Tracked");

struct alignas(16) Aligned16 {
    std::array<float, 4> values;
};
RTYPE_ECS_COMPONENT(Aligned16, "test.Aligned16", values);

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "test.Dead");
}  // namespace rtype::test

namespace {
template <typename T>
rtype::ecs::storage::ByteColumn makeColumn() {
    return rtype::ecs::storage::ByteColumn{rtype::ecs::ComponentTraits<T>::makeInfo()};
}

template <typename T>
T& valueAt(rtype::ecs::storage::ByteColumn& column, std::size_t position) {
    return *static_cast<T*>(column.at(position));
}
}  // namespace

TEST(ByteColumn, StartsEmpty) {
    const rtype::ecs::storage::ByteColumn column = makeColumn<rtype::test::Plain>();
    EXPECT_EQ(column.getSize(), 0U);
    EXPECT_EQ(column.getCapacity(), 0U);
    EXPECT_EQ(column.getElementSize(), sizeof(rtype::test::Plain));
}

TEST(ByteColumn, EmplaceBackZeroFillsPlainData) {
    rtype::ecs::storage::ByteColumn column = makeColumn<rtype::test::Plain>();
    const auto* value = static_cast<const rtype::test::Plain*>(column.emplaceBack());
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->a, 0);
    EXPECT_EQ(value->b, 0.0F);
    EXPECT_EQ(column.getSize(), 1U);
}

TEST(ByteColumn, EmplaceBackAppliesDefaultMemberInitializers) {
    rtype::ecs::storage::ByteColumn column = makeColumn<rtype::test::Scaled>();
    EXPECT_EQ(static_cast<const rtype::test::Scaled*>(column.emplaceBack())->scale, 1.0F);
}

TEST(ByteColumn, KeepsValuesWhenGrowing) {
    rtype::ecs::storage::ByteColumn column = makeColumn<rtype::test::Plain>();
    for (std::int32_t i = 0; i < 100; ++i) {
        const rtype::test::Plain value{.a = i, .b = static_cast<float>(i) / 2.0F};
        column.emplaceBackCopy(&value);
    }
    ASSERT_EQ(column.getSize(), 100U);
    EXPECT_GE(column.getCapacity(), 100U);
    for (std::int32_t i = 0; i < 100; ++i) {
        EXPECT_EQ(valueAt<rtype::test::Plain>(column, static_cast<std::size_t>(i)).a, i);
    }
}

TEST(ByteColumn, CopiesAndMovesValuesIn) {
    rtype::ecs::storage::ByteColumn column = makeColumn<rtype::test::Tracked>();
    rtype::test::Tracked source{.items = {1, 2, 3}};
    column.emplaceBackCopy(&source);
    EXPECT_EQ(source.items.size(), 3U);  // copied, untouched
    column.emplaceBackMove(&source);
    EXPECT_EQ(valueAt<rtype::test::Tracked>(column, 0).items, (std::vector<std::int32_t>{1, 2, 3}));
    EXPECT_EQ(valueAt<rtype::test::Tracked>(column, 1).items, (std::vector<std::int32_t>{1, 2, 3}));
}

TEST(ByteColumn, SwapRemoveMovesTheLastValueIntoTheHole) {
    rtype::ecs::storage::ByteColumn column = makeColumn<rtype::test::Plain>();
    for (std::int32_t i = 0; i < 3; ++i) {
        const rtype::test::Plain value{.a = i, .b = 0.0F};
        column.emplaceBackCopy(&value);
    }
    column.swapRemove(0);  // 2 moves into position 0
    ASSERT_EQ(column.getSize(), 2U);
    EXPECT_EQ(valueAt<rtype::test::Plain>(column, 0).a, 2);
    EXPECT_EQ(valueAt<rtype::test::Plain>(column, 1).a, 1);
    column.swapRemove(1);  // the last one: nothing moves
    ASSERT_EQ(column.getSize(), 1U);
    EXPECT_EQ(valueAt<rtype::test::Plain>(column, 0).a, 2);
}

TEST(ByteColumn, NonTrivialValuesSurviveGrowthAndRemoval) {
    liveCounters = 0;
    {
        rtype::ecs::storage::ByteColumn column = makeColumn<rtype::test::Tracked>();
        for (std::int32_t i = 0; i < 50; ++i) {
            auto* value = static_cast<rtype::test::Tracked*>(column.emplaceBack());
            value->items = {i, i + 1};
        }
        EXPECT_EQ(liveCounters, 50);
        column.swapRemove(10);  // 49 moves into position 10
        EXPECT_EQ(valueAt<rtype::test::Tracked>(column, 10).items, (std::vector<std::int32_t>{49, 50}));
        EXPECT_EQ(liveCounters, 49);
        column.clear();
        EXPECT_EQ(liveCounters, 0);
        column.emplaceBack();
        EXPECT_EQ(liveCounters, 1);
    }
    EXPECT_EQ(liveCounters, 0);  // the destructor destroyed the last value
}

TEST(ByteColumn, KeepsValuesAlignedAfterGrowth) {
    rtype::ecs::storage::ByteColumn column = makeColumn<rtype::test::Aligned16>();
    for (int i = 0; i < 200; ++i) {
        column.emplaceBack();
    }
    for (std::size_t position = 0; position < column.getSize(); ++position) {
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(column.at(position)) % 16, 0U) << "position " << position;
    }
}

TEST(ByteColumn, TagsOnlyCount) {
    CountingResource resource;
    {
        rtype::ecs::storage::ByteColumn column{rtype::ecs::ComponentTraits<rtype::test::Dead>::makeInfo(), &resource};
        EXPECT_EQ(column.emplaceBack(), nullptr);
        column.emplaceBack();
        EXPECT_EQ(column.getSize(), 2U);
        EXPECT_EQ(column.at(0), nullptr);
        column.swapRemove(0);
        EXPECT_EQ(column.getSize(), 1U);
    }
    EXPECT_EQ(resource.allocations, 0);
}

TEST(ByteColumn, GivesMemoryBackToItsResource) {
    CountingResource resource;
    {
        rtype::ecs::storage::ByteColumn column{rtype::ecs::ComponentTraits<rtype::test::Plain>::makeInfo(), &resource};
        for (int i = 0; i < 100; ++i) {
            column.emplaceBack();
        }
        column.reserve(1000);
    }
    EXPECT_GT(resource.allocations, 1);
    EXPECT_EQ(resource.deallocations, resource.allocations);
    EXPECT_FALSE(resource.mismatch);
}

TEST(ByteColumn, MovingTransfersTheValues) {
    liveCounters = 0;
    {
        rtype::ecs::storage::ByteColumn column = makeColumn<rtype::test::Tracked>();
        static_cast<rtype::test::Tracked*>(column.emplaceBack())->items = {7};
        rtype::ecs::storage::ByteColumn moved{std::move(column)};
        EXPECT_EQ(moved.getSize(), 1U);
        EXPECT_EQ(valueAt<rtype::test::Tracked>(moved, 0).items, (std::vector<std::int32_t>{7}));
        EXPECT_EQ(column.getSize(), 0U);  // NOLINT(bugprone-use-after-move): moved-from is specified empty
        column.emplaceBack();             // the moved-from column is still usable
        rtype::ecs::storage::ByteColumn assigned = makeColumn<rtype::test::Tracked>();
        assigned.emplaceBack();
        assigned = std::move(moved);  // destroys assigned's own value first
        EXPECT_EQ(assigned.getSize(), 1U);
        EXPECT_EQ(liveCounters, 2);  // assigned's value + the one in the moved-from column
    }
    EXPECT_EQ(liveCounters, 0);
}
