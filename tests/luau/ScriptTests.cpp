/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ScriptTests
*/

#include <gtest/gtest.h>

#include <type_traits>
#include <utility>

#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Script.hpp"

namespace {
rtype::luau::Runtime makeRuntime() {
    auto runtime = rtype::luau::Runtime::create({
        .libs = rtype::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}
}  // namespace

TEST(Script, ExposesName) {
    const auto runtime = makeRuntime();

    const auto script = runtime.load("main.luau", "local x = 1");

    ASSERT_TRUE(script);
    EXPECT_EQ(script->name(), "main.luau");
    EXPECT_NE(script->state(), nullptr);
}

TEST(Script, MoveConstructionTransfersParts) {
    const auto runtime = makeRuntime();
    auto source = runtime.load("moved.luau", "assert(1 + 1 == 2)");
    ASSERT_TRUE(source);

    const rtype::luau::Script moved{std::move(*source)};

    EXPECT_EQ(moved.name(), "moved.luau");
    EXPECT_TRUE(runtime.run(moved));
}

TEST(Script, IsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<rtype::luau::Script>);
    static_assert(!std::is_copy_assignable_v<rtype::luau::Script>);
    static_assert(std::is_nothrow_move_constructible_v<rtype::luau::Script>);
    static_assert(std::is_nothrow_move_assignable_v<rtype::luau::Script>);
    SUCCEED();
}