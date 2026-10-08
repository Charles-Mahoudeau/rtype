/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResultTests
*/

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <tl/expected.hpp>
#include <type_traits>
#include <utility>

#include "rtype/luau/Error.hpp"
#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Result.hpp"

namespace {
rtype::luau::Result<int> makeFailure(const rtype::luau::ErrorKind kind, std::string message) {
    return tl::unexpected{rtype::luau::Error{kind, std::move(message)}};
}
}  // namespace

TEST(Result, SuccessConvertsToTrue) {
    const rtype::luau::Result<int> result{42};

    EXPECT_TRUE(result);
    EXPECT_TRUE(result.has_value());
}

TEST(Result, FailureConvertsToFalse) {
    const auto result = makeFailure(rtype::luau::ErrorKind::kRuntime, "boom");

    EXPECT_FALSE(result);
    EXPECT_FALSE(result.has_value());
}

TEST(Result, DereferenceReturnsValue) {
    const rtype::luau::Result<int> result{42};

    EXPECT_EQ(*result, 42);
}

TEST(Result, ArrowOperatorAccessesValueMembers) {
    const rtype::luau::Result<std::string> result{"hello"};

    ASSERT_TRUE(result);
    EXPECT_EQ(result->size(), 5U);
}

TEST(Result, DereferenceAllowsMutation) {
    rtype::luau::Result<int> result{1};

    *result = 7;

    EXPECT_EQ(*result, 7);
}

TEST(Result, ErrorExposesKindAndMessage) {
    const auto result = makeFailure(rtype::luau::ErrorKind::kTimeout, "took too long");

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kTimeout);
    EXPECT_EQ(result.error().message(), "took too long");
}

TEST(Result, AcceptsErrorBuiltWithTraceback) {
    const rtype::luau::Result<int> result{
        tl::unexpected{rtype::luau::Error{rtype::luau::ErrorKind::kRuntime, "boom", "script:3"}}};

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
}

TEST(Result, WorksWithCopyableType) {
    static_assert(std::is_copy_constructible_v<std::string>);

    const rtype::luau::Result<std::string> result{"copy me"};
    std::string copy = *result;
    copy += "!";

    EXPECT_EQ(copy, "copy me!");
    EXPECT_EQ(*result, "copy me");
}

TEST(Result, WorksWithMoveOnlyType) {
    rtype::luau::Result<std::unique_ptr<int>> result{std::make_unique<int>(9)};

    ASSERT_TRUE(result);
    EXPECT_EQ(**result, 9);

    const std::unique_ptr<int> taken = std::move(*result);
    ASSERT_NE(taken, nullptr);
    EXPECT_EQ(*taken, 9);
}

TEST(Result, MoveOnlyResultIsMovable) {
    rtype::luau::Result<std::unique_ptr<int>> source{std::make_unique<int>(3)};
    rtype::luau::Result<std::unique_ptr<int>> target{std::move(source)};

    ASSERT_TRUE(target);
    EXPECT_EQ(**target, 3);
}

TEST(Result, MoveOnlyFailureIsMovable) {
    rtype::luau::Result<std::unique_ptr<int>> source{
        tl::unexpected{rtype::luau::Error{rtype::luau::ErrorKind::kOutOfMemory, "oom"}}};
    const rtype::luau::Result<std::unique_ptr<int>> target{std::move(source)};

    ASSERT_FALSE(target);
    EXPECT_EQ(target.error().kind(), rtype::luau::ErrorKind::kOutOfMemory);
    EXPECT_EQ(target.error().message(), "oom");
}

TEST(ResultVoid, DefaultIsSuccess) {
    const rtype::luau::Result<> result{};

    EXPECT_TRUE(result);
    EXPECT_TRUE(result.has_value());
}

TEST(ResultVoid, ExplicitVoidIsSuccess) {
    const rtype::luau::Result<void> result{};

    EXPECT_TRUE(result);
}

TEST(ResultVoid, FailureConvertsToFalse) {
    const rtype::luau::Result<> result{
        tl::unexpected{rtype::luau::Error{rtype::luau::ErrorKind::kSyntax, "bad chunk"}}};

    EXPECT_FALSE(result);
}

TEST(ResultVoid, ErrorExposesKindAndMessage) {
    const rtype::luau::Result<> result{
        tl::unexpected{rtype::luau::Error{rtype::luau::ErrorKind::kStackOverflow, "too deep"}}};

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kStackOverflow);
    EXPECT_EQ(result.error().message(), "too deep");
}

TEST(Result, ForwardErrorKeepsKindAndMessage) {
    auto source = makeFailure(rtype::luau::ErrorKind::kCompilation, "bad syntax");

    const auto forwarded = rtype::luau::forwardError<std::string>(std::move(source));

    ASSERT_FALSE(forwarded);
    EXPECT_EQ(forwarded.error().kind(), rtype::luau::ErrorKind::kCompilation);
    EXPECT_EQ(forwarded.error().message(), "bad syntax");
}

TEST(Result, ForwardErrorToVoidResult) {
    auto source = makeFailure(rtype::luau::ErrorKind::kRuntime, "boom");

    const auto forwarded = rtype::luau::forwardError<void>(std::move(source));

    ASSERT_FALSE(forwarded);
    EXPECT_EQ(forwarded.error().kind(), rtype::luau::ErrorKind::kRuntime);
}
