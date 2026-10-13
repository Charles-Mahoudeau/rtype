/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PagedSparseArrayTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

#include "rtype/ecs/storage/PagedSparseArray.hpp"

namespace {
constexpr std::uint32_t kMaxIndex = std::numeric_limits<std::uint32_t>::max();
constexpr std::uint32_t kPageSize = rtype::ecs::storage::PagedSparseArray::kPageSize;
}  // namespace

TEST(PagedSparseArray, StartsEmpty) {
    const rtype::ecs::storage::PagedSparseArray sparse;
    EXPECT_EQ(sparse.getAllocatedPageCount(), 0U);
    EXPECT_FALSE(sparse.get(0));
    EXPECT_FALSE(sparse.get(12345));
    EXPECT_FALSE(sparse.get(kMaxIndex));
}

TEST(PagedSparseArray, ReturnsWhatWasSet) {
    rtype::ecs::storage::PagedSparseArray sparse;
    sparse.set(7, 3);
    EXPECT_EQ(sparse.get(7), std::optional<std::uint32_t>{3});
    sparse.set(7, 0);  // position 0 is a valid position
    EXPECT_EQ(sparse.get(7), std::optional<std::uint32_t>{0});
}

TEST(PagedSparseArray, IndicesNeverSetAreEmpty) {
    rtype::ecs::storage::PagedSparseArray sparse;
    sparse.set(10, 1);
    EXPECT_FALSE(sparse.get(9));
    EXPECT_FALSE(sparse.get(11));
    EXPECT_FALSE(sparse.get(10 + kPageSize));  // a page that was never allocated
}

TEST(PagedSparseArray, AllocatesOnlyTouchedPages) {
    rtype::ecs::storage::PagedSparseArray sparse;
    sparse.set(0, 1);
    sparse.set(kPageSize - 1, 2);  // same page as 0
    EXPECT_EQ(sparse.getAllocatedPageCount(), 1U);
    sparse.set(5000, 3);  // second page
    EXPECT_EQ(sparse.getAllocatedPageCount(), 2U);
    sparse.set(100 * kPageSize, 4);  // far away: pages in between stay unallocated
    EXPECT_EQ(sparse.getAllocatedPageCount(), 3U);
}

TEST(PagedSparseArray, HandlesVeryHighIndices) {
    rtype::ecs::storage::PagedSparseArray sparse;
    sparse.set(kMaxIndex - 1, 42);
    EXPECT_EQ(sparse.get(kMaxIndex - 1), std::optional<std::uint32_t>{42});
    EXPECT_FALSE(sparse.get(kMaxIndex));
    EXPECT_EQ(sparse.getAllocatedPageCount(), 1U);
}

TEST(PagedSparseArray, NeighbouringPagesAreIndependent) {
    rtype::ecs::storage::PagedSparseArray sparse;
    sparse.set(kPageSize - 1, 1);
    sparse.set(kPageSize, 2);
    EXPECT_EQ(sparse.get(kPageSize - 1), std::optional<std::uint32_t>{1});
    EXPECT_EQ(sparse.get(kPageSize), std::optional<std::uint32_t>{2});
}

TEST(PagedSparseArray, EraseEmptiesTheEntryAndKeepsThePage) {
    rtype::ecs::storage::PagedSparseArray sparse;
    sparse.set(5, 1);
    sparse.set(6, 2);
    sparse.erase(5);
    EXPECT_FALSE(sparse.get(5));
    EXPECT_EQ(sparse.get(6), std::optional<std::uint32_t>{2});
    EXPECT_EQ(sparse.getAllocatedPageCount(), 1U);
}

TEST(PagedSparseArray, ErasingAbsentIndicesDoesNothing) {
    rtype::ecs::storage::PagedSparseArray sparse;
    sparse.erase(3);          // nothing allocated at all
    sparse.erase(kMaxIndex);  // far beyond the page table
    EXPECT_EQ(sparse.getAllocatedPageCount(), 0U);
    sparse.set(1, 9);
    sparse.erase(2);  // allocated page, empty entry
    EXPECT_EQ(sparse.get(1), std::optional<std::uint32_t>{9});
}

TEST(PagedSparseArray, ClearEmptiesEveryEntryAndKeepsThePages) {
    rtype::ecs::storage::PagedSparseArray sparse;
    sparse.set(1, 1);
    sparse.set(2 * kPageSize, 2);
    sparse.clear();
    EXPECT_FALSE(sparse.get(1));
    EXPECT_FALSE(sparse.get(2 * kPageSize));
    EXPECT_EQ(sparse.getAllocatedPageCount(), 2U);
}

TEST(PagedSparseArray, KeepsItsEntriesWhenMoved) {
    rtype::ecs::storage::PagedSparseArray sparse;
    sparse.set(3, 30);
    rtype::ecs::storage::PagedSparseArray moved{std::move(sparse)};
    EXPECT_EQ(moved.get(3), std::optional<std::uint32_t>{30});
    EXPECT_EQ(moved.getAllocatedPageCount(), 1U);

    rtype::ecs::storage::PagedSparseArray assigned;
    assigned = std::move(moved);
    EXPECT_EQ(assigned.get(3), std::optional<std::uint32_t>{30});
}
