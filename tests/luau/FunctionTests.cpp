/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <utility>

#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/Ref.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"

namespace {
using CallContext = rtype::luau::CallContext;
using Function = rtype::luau::Function;
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

/// @brief Wraps the chunk of a Luau source in a Function. The chunk receives the call arguments as `...`.
Function makeFunction(const rtype::luau::Runtime& runtime, const std::string& source) {
    const auto script = runtime.load("function", source);
    EXPECT_TRUE(script);
    script->closure().push(runtime.state());
    return Function::fromRef(rtype::luau::Ref::pop(runtime.state()), "function");
}

/// @brief Builds a context whose last argument cannot be pushed, after @p validArgs pushable ones.
CallContext makeUnpushableContext(const std::size_t validArgs) {
    CallContext context{};
    for (std::size_t i = 0; i < validArgs; ++i) {
        context.addArg({0.0});
    }
    context.addArg({rtype::luau::Table{}});
    return context;
}
}  // namespace

TEST(Function, CallWithDefaultContextPassesNoArguments) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, R"(assert(select("#", ...) == 0))");

    EXPECT_TRUE(function.call());
}

TEST(Function, CallPassesArgumentsInOrder) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, R"(
        local a, b, c = ...
        assert(select("#", ...) == 3)
        assert(a == "first" and b == 2 and c == true)
    )");
    CallContext context{};
    context.addArg({std::string{"first"}});
    context.addArg({2.0});
    context.addArg({true});

    EXPECT_TRUE(function.call(context));
}

TEST(Function, CallPassesEveryPushableType) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, R"(
        local n, b, d, v, s = ...
        assert(select("#", ...) == 5)
        assert(n == nil)
        assert(b == false)
        assert(d == 1.5)
        assert(typeof(v) == "vector" and v.x == 1 and v.y == 2 and v.z == 3)
        assert(s == "text")
    )");
    CallContext context{};
    context.addArg({Value::Nil{}});
    context.addArg({false});
    context.addArg({1.5});
    context.addArg({Value::Vector{1.0F, 2.0F, 3.0F}});
    context.addArg({std::string{"text"}});

    EXPECT_TRUE(function.call(context));
}

TEST(Function, CallPassesMoreArgumentsThanMinimumStack) {
    const auto runtime = makeRuntime();
    constexpr std::size_t kArgCount = 1000;
    const Function function = makeFunction(runtime, R"(
        assert(select("#", ...) == 1000)
        assert(select(1000, ...) == 999)
    )");
    CallContext context{};
    for (std::size_t i = 0; i < kArgCount; ++i) {
        context.addArg({static_cast<double>(i)});
    }

    EXPECT_TRUE(function.call(context));
}

TEST(Function, CallFailsWhenFunctionFails) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, R"(local a = ...; assert(a == "expected"))");
    CallContext context{};
    context.addArg({std::string{"unexpected"}});

    const auto result = function.call(context);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
}

TEST(Function, CallFailsWithTooManyArguments) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, "error('should not run')");
    CallContext context{};
    for (std::size_t i = 0; i < kTooManyArgs; ++i) {
        context.addArg({0.0});
    }

    const auto result = function.call(context);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
    EXPECT_EQ(result.error().message(), "Lua stack overflow");
}

TEST(Function, CallFailsForTableArgument) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, "error('should not run')");

    const auto result = function.call(makeUnpushableContext(0));

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
    EXPECT_EQ(result.error().message(), "not implemented");
}

TEST(Function, CallWorksAfterFailedPush) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, R"(local a = ...; assert(a == "ok"))");
    EXPECT_FALSE(function.call(makeUnpushableContext(3)));
    CallContext context{};
    context.addArg({std::string{"ok"}});

    EXPECT_TRUE(function.call(context));
}

TEST(Function, FailedPushDoesNotLeaveValuesOnStack) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, "error('should not run')");
    // Each failed call pushes the function and 100 arguments. If they were left on the stack, they would add
    // up past Luau's 8000-slot cap within 100 calls, and pushArgs would fail with a stack overflow instead.
    constexpr int kCalls = 100;
    const CallContext context = makeUnpushableContext(100);

    for (int i = 0; i < kCalls; ++i) {
        const auto result = function.call(context);
        ASSERT_FALSE(result) << "call " << i;
        ASSERT_EQ(result.error().message(), "not implemented") << "call " << i;
    }
}
