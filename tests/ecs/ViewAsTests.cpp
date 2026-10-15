/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ViewAsTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <span>

#include "rtype/ecs/storage/ViewAs.hpp"

namespace {
struct Pair {
    std::int32_t first;
    float second;
};
}  // namespace

TEST(ViewAs, ViewsTheValueHeldByTheBytes) {
    Pair pair{.first = 7, .second = 1.5F};
    Pair* viewed = rtype::ecs::storage::viewAs<Pair>(std::as_writable_bytes(std::span{&pair, 1}));
    EXPECT_EQ(viewed, &pair);
    EXPECT_EQ(viewed->first, 7);
    EXPECT_EQ(viewed->second, 1.5F);
}

TEST(ViewAs, WritesThroughTheView) {
    Pair pair{.first = 0, .second = 0.0F};
    rtype::ecs::storage::viewAs<Pair>(std::as_writable_bytes(std::span{&pair, 1}))->first = 12;
    EXPECT_EQ(pair.first, 12);
}

TEST(ViewAs, ViewsReadOnlyBytesAsAConstValue) {
    const Pair pair{.first = 3, .second = 2.0F};
    const Pair* viewed = rtype::ecs::storage::viewAs<Pair>(std::as_bytes(std::span{&pair, 1}));
    EXPECT_EQ(viewed, &pair);
    EXPECT_EQ(viewed->first, 3);
}
