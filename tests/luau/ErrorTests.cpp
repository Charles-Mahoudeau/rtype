/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ErrorTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <exception>
#include <source_location>
#include <string>
#include <type_traits>
#include <utility>

#include "rtype/luau/Error.hpp"
#include "rtype/luau/ErrorKind.hpp"

TEST(Error, StoresKindAndMessage) {
    const rtype::luau::Error error{rtype::luau::ErrorKind::kSyntax, "unexpected symbol"};

    EXPECT_EQ(error.kind(), rtype::luau::ErrorKind::kSyntax);
    EXPECT_EQ(error.message(), "unexpected symbol");
}

TEST(Error, StoresEveryKind) {
    constexpr std::array kinds{
        rtype::luau::ErrorKind::kRuntime, rtype::luau::ErrorKind::kSyntax,      rtype::luau::ErrorKind::kTypeMismatch,
        rtype::luau::ErrorKind::kTimeout, rtype::luau::ErrorKind::kOutOfMemory, rtype::luau::ErrorKind::kStackOverflow,
        rtype::luau::ErrorKind::kUnknown,
    };

    for (const auto kind : kinds) {
        const rtype::luau::Error error{kind, "message"};
        EXPECT_EQ(error.kind(), kind);
    }
}

TEST(Error, KindsAreDistinct) {
    EXPECT_NE(rtype::luau::ErrorKind::kRuntime, rtype::luau::ErrorKind::kSyntax);
    EXPECT_NE(rtype::luau::ErrorKind::kTimeout, rtype::luau::ErrorKind::kOutOfMemory);
    EXPECT_NE(rtype::luau::ErrorKind::kStackOverflow, rtype::luau::ErrorKind::kUnknown);
}

TEST(Error, AcceptsEmptyMessage) {
    const rtype::luau::Error error{rtype::luau::ErrorKind::kUnknown, ""};

    EXPECT_TRUE(error.message().empty());
    EXPECT_STREQ(error.what(), "");
}

TEST(Error, WhatMatchesMessage) {
    const rtype::luau::Error error{rtype::luau::ErrorKind::kRuntime, "boom"};

    EXPECT_STREQ(error.what(), "boom");
    EXPECT_EQ(error.message(), error.what());
}

TEST(Error, WhatIsNullTerminatedAndStable) {
    const rtype::luau::Error error{rtype::luau::ErrorKind::kRuntime, "boom"};

    EXPECT_EQ(error.what(), error.what());
    EXPECT_EQ(std::strlen(error.what()), error.message().size());
}

TEST(Error, IsAStdException) {
    static_assert(std::is_base_of_v<std::exception, rtype::luau::Error>);

    const rtype::luau::Error error{rtype::luau::ErrorKind::kRuntime, "boom"};
    const std::exception& base = error;
    EXPECT_STREQ(base.what(), "boom");
}

TEST(Error, AcceptsTraceback) {
    const rtype::luau::Error error{rtype::luau::ErrorKind::kRuntime, "boom", "script:1\nscript:2"};

    EXPECT_EQ(error.kind(), rtype::luau::ErrorKind::kRuntime);
    EXPECT_EQ(error.message(), "boom");
}

TEST(Error, AcceptsExplicitSourceLocation) {
    const rtype::luau::Error error{rtype::luau::ErrorKind::kTimeout, "too slow", std::source_location::current()};

    EXPECT_EQ(error.kind(), rtype::luau::ErrorKind::kTimeout);
    EXPECT_EQ(error.message(), "too slow");
}

TEST(Error, AcceptsTracebackAndSourceLocation) {
    const rtype::luau::Error error{rtype::luau::ErrorKind::kStackOverflow, "deep", "trace",
                                   std::source_location::current()};

    EXPECT_EQ(error.kind(), rtype::luau::ErrorKind::kStackOverflow);
    EXPECT_EQ(error.message(), "deep");
}

TEST(Error, KeepsLongMessage) {
    const std::string message(4096, 'x');
    const rtype::luau::Error error{rtype::luau::ErrorKind::kOutOfMemory, message};

    EXPECT_EQ(error.message(), message);
    EXPECT_STREQ(error.what(), message.c_str());
}

TEST(Error, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<rtype::luau::Error>);
    static_assert(!std::is_copy_assignable_v<rtype::luau::Error>);
    static_assert(std::is_nothrow_move_constructible_v<rtype::luau::Error>);
    static_assert(std::is_nothrow_move_assignable_v<rtype::luau::Error>);
}

TEST(Error, MoveConstructionKeepsState) {
    rtype::luau::Error original{rtype::luau::ErrorKind::kTypeMismatch, "expected number"};
    const rtype::luau::Error moved{std::move(original)};

    EXPECT_EQ(moved.kind(), rtype::luau::ErrorKind::kTypeMismatch);
    EXPECT_EQ(moved.message(), "expected number");
}

TEST(Error, MoveAssignmentKeepsState) {
    rtype::luau::Error source{rtype::luau::ErrorKind::kSyntax, "bad token"};
    rtype::luau::Error target{rtype::luau::ErrorKind::kRuntime, "other"};

    target = std::move(source);

    EXPECT_EQ(target.kind(), rtype::luau::ErrorKind::kSyntax);
    EXPECT_EQ(target.message(), "bad token");
}
