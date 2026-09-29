/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FailureTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <type_traits>

#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Failure.hpp"
#include "rtype/luau/Result.hpp"

namespace {
rtype::luau::Result<int> failWithValue() { return rtype::luau::Failure{rtype::luau::ErrorKind::kRuntime, "boom"}; }

rtype::luau::Result<> failWithVoid() { return rtype::luau::Failure{rtype::luau::ErrorKind::kSyntax, "bad chunk"}; }

rtype::luau::Result<std::unique_ptr<int>> failWithMoveOnly() {
    return rtype::luau::Failure{rtype::luau::ErrorKind::kOutOfMemory, "oom"};
}
}  // namespace

TEST(Failure, ConvertsToValueResult) {
    const auto result = failWithValue();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
    EXPECT_EQ(result.error().message(), "boom");
}

TEST(Failure, ConvertsToVoidResult) {
    const auto result = failWithVoid();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kSyntax);
    EXPECT_EQ(result.error().message(), "bad chunk");
}

TEST(Failure, ConvertsToMoveOnlyResult) {
    const auto result = failWithMoveOnly();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kOutOfMemory);
}

TEST(Failure, AcceptsTraceback) {
    const rtype::luau::Result<int> result = rtype::luau::Failure{rtype::luau::ErrorKind::kRuntime, "boom", "script:3"};

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
}

TEST(Failure, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<rtype::luau::Failure>);
    static_assert(!std::is_copy_assignable_v<rtype::luau::Failure>);
    static_assert(std::is_move_constructible_v<rtype::luau::Failure>);
    static_assert(std::is_move_assignable_v<rtype::luau::Failure>);
}
