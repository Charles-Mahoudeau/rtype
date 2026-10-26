/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CallContextTests
*/

#include <gtest/gtest.h>
#include <lua.h>

#include <string>
#include <utility>

#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
using CallContext = rtype::luau::CallContext;
using ValuePack = rtype::luau::ValuePack;

/// @brief Creates a runtime with the standard libraries, failing the test if it cannot be created.
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
    EXPECT_TRUE(context.args().empty());
}

TEST(CallContext, KeepsArgumentsInOrder) {
    ValuePack args{};
    args.add({1.0});
    args.add({std::string{"two"}});

    const CallContext context{std::move(args)};

    ASSERT_EQ(context.argCount(), 2U);
    EXPECT_EQ(context.args()[0].as<double>(), 1.0);
    EXPECT_EQ(context.args()[1].as<std::string>(), "two");
}

TEST(CallContext, MoveKeepsArguments) {
    ValuePack args{};
    args.add({true});
    CallContext context{std::move(args)};

    const CallContext moved{std::move(context)};

    EXPECT_EQ(moved.argCount(), 1U);
}

TEST(CallContext, FromStackReadsWholeStackInOrder) {
    const auto runtime = makeRuntime();
    ValuePack pushed{};
    pushed.add({std::string{"first"}});
    pushed.add({2.0});
    pushed.add({true});
    ASSERT_EQ(pushed.push(runtime.state()).value(), 3U);

    const auto context = CallContext::fromStack(runtime.state());

    ASSERT_TRUE(context);
    ASSERT_EQ(context->argCount(), 3U);
    EXPECT_EQ(context->args()[0].as<std::string>(), "first");
    EXPECT_EQ(context->args()[1].as<double>(), 2.0);
    EXPECT_EQ(context->args()[2].as<bool>(), true);
}

TEST(CallContext, FromStackOfEmptyStackHasNoArguments) {
    const auto runtime = makeRuntime();

    const auto context = CallContext::fromStack(runtime.state());

    ASSERT_TRUE(context);
    EXPECT_EQ(context->argCount(), 0U);
}

TEST(CallContext, FromStackFailsForUnreadableArgument) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("chunk", "local _ = 1");
    ASSERT_TRUE(script);
    // A thread cannot be read yet.
    lua_newthread(runtime.state());

    const auto context = CallContext::fromStack(runtime.state());

    ASSERT_FALSE(context);
    EXPECT_EQ(context.error().kind(), rtype::luau::ErrorKind::kTypeMismatch);
}
