/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** RuntimeTests
*/

#include <gtest/gtest.h>

#include <utility>

#include "rtype/luau/Runtime.hpp"

TEST(Runtime, CreateReturnsRuntime) {
    const auto runtime = rtype::luau::Runtime::create();
    EXPECT_TRUE(runtime.has_value());
}

TEST(Runtime, IsMovable) {
    auto runtime = rtype::luau::Runtime::create();
    ASSERT_TRUE(runtime.has_value());

    rtype::luau::Runtime moved{std::move(*runtime)};
    auto other = rtype::luau::Runtime::create();
    ASSERT_TRUE(other.has_value());
    *other = std::move(moved);
}
