/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallContextTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <utility>

#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"

namespace {
using CallContext = rtype::luau::CallContext;
using Value = rtype::luau::Value;

/// @brief More arguments than a Lua stack can hold (Luau caps it at 8000 slots).
constexpr std::size_t kTooManyArgs = 10000;

rtype::luau::Runtime makeRuntime() {
    auto runtime = rtype::luau::Runtime::create({
        .libs = rtype::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}
}  // namespace

TEST(CallContext, IsEmptyByDefault) {
    const CallContext context{};

    EXPECT_EQ(context.argCount(), 0U);
}

TEST(CallContext, AddArgIncrementsArgCount) {
    CallContext context{};

    context.addArg({1.0});
    context.addArg({std::string{"two"}});

    EXPECT_EQ(context.argCount(), 2U);
}

TEST(CallContext, MoveKeepsArguments) {
    CallContext context{};
    context.addArg({true});

    const CallContext moved{std::move(context)};

    EXPECT_EQ(moved.argCount(), 1U);
}

TEST(CallContext, PushArgsReturnsArgCount) {
    const auto runtime = makeRuntime();
    CallContext context{};
    context.addArg({Value::Nil{}});
    context.addArg({false});
    context.addArg({3.0});

    EXPECT_EQ(context.pushArgs(runtime.state()), 3U);
}

TEST(CallContext, PushArgsWithoutArgumentsReturnsZero) {
    const auto runtime = makeRuntime();
    const CallContext context{};

    EXPECT_EQ(context.pushArgs(runtime.state()), 0U);
}

TEST(CallContext, PushArgsGrowsStackBeyondMinimum) {
    const auto runtime = makeRuntime();
    CallContext context{};
    // Well above LUA_MINSTACK (20): pushing them without growing the stack would overflow it.
    constexpr std::size_t kArgCount = 1000;
    for (std::size_t i = 0; i < kArgCount; ++i) {
        context.addArg({static_cast<double>(i)});
    }

    EXPECT_EQ(context.pushArgs(runtime.state()), kArgCount);
}

TEST(CallContext, PushArgsThrowsStackOverflowWhenStackCannotGrow) {
    const auto runtime = makeRuntime();
    CallContext context{};
    for (std::size_t i = 0; i < kTooManyArgs; ++i) {
        context.addArg({0.0});
    }

    EXPECT_THROW(static_cast<void>(context.pushArgs(runtime.state())), rtype::luau::exceptions::StackOverflow);
}

TEST(CallContext, PushArgsThrowsNotImplementedForTable) {
    const auto runtime = makeRuntime();
    CallContext context{};
    context.addArg({rtype::luau::Table{}});

    EXPECT_THROW(static_cast<void>(context.pushArgs(runtime.state())), rtype::luau::exceptions::NotImplemented);
}
