/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ErrorTests
*/

#include <gtest/gtest.h>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"

namespace {
rtype::ecs::Result<int> parsePositive(int value) {
    if (value < 0) {
        return rtype::ecs::fail(rtype::ecs::ErrorKind::kInvalidComponent, "negative");
    }
    return value;
}

rtype::ecs::Result<void> check(bool ok) {
    if (!ok) {
        return rtype::ecs::fail(rtype::ecs::ErrorKind::kInvalidComponent, "not ok");
    }
    return {};
}
}  // namespace

TEST(Error, KeepsKindAndMessage) {
    const rtype::ecs::Error error{rtype::ecs::ErrorKind::kInvalidComponent, "broken"};
    EXPECT_EQ(error.getKind(), rtype::ecs::ErrorKind::kInvalidComponent);
    EXPECT_EQ(error.getMessage(), "broken");
}

TEST(Error, IsCopyable) {
    const rtype::ecs::Error error{rtype::ecs::ErrorKind::kInvalidComponent, "broken"};
    const rtype::ecs::Error copy{error};  // NOLINT(performance-unnecessary-copy-initialization)
    EXPECT_EQ(copy.getMessage(), error.getMessage());
}

TEST(Result, HoldsAValueOnSuccess) {
    const rtype::ecs::Result<int> result = parsePositive(3);
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, 3);
    EXPECT_TRUE(check(true));
}

TEST(Result, FailBuildsTheErrorSide) {
    const rtype::ecs::Result<int> value = parsePositive(-1);
    ASSERT_FALSE(value);
    EXPECT_EQ(value.error().getKind(), rtype::ecs::ErrorKind::kInvalidComponent);
    EXPECT_EQ(value.error().getMessage(), "negative");

    const rtype::ecs::Result<void> none = check(false);
    ASSERT_FALSE(none);
    EXPECT_EQ(none.error().getMessage(), "not ok");
}
