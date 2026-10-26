/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionRefTests
*/

#include <gtest/gtest.h>
#include <lua.h>

#include <string>
#include <utility>

#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/FunctionRef.hpp"
#include "rtype/luau/Ref.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
using ErrorKind = rtype::luau::ErrorKind;
using Function = rtype::luau::Function;
using FunctionRef = rtype::luau::FunctionRef;
using Value = rtype::luau::Value;
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

/// @brief Evaluates @p source, which must return a function, and wraps that function in a handle.
FunctionRef makeFunctionRef(const rtype::luau::Runtime& runtime, const std::string& source) {
    const auto script = runtime.load("function-ref", source);
    EXPECT_TRUE(script);
    script->closure().push(runtime.state());
    const Function loader = Function::fromRef(rtype::luau::Ref::pop(runtime.state()), "loader");
    auto results = loader.call();
    EXPECT_TRUE(results);
    EXPECT_EQ(results->values().size(), 1U);
    auto function = results->values()[0].tryAs<FunctionRef>();
    EXPECT_NE(function, nullptr);
    return *function;
}
}  // namespace

TEST(FunctionRef, FromStackReadsFunction) {
    const auto runtime = makeRuntime();
    const auto function = makeFunctionRef(runtime, "return function(a) return a * 2 end");

    ASSERT_TRUE(function.push(runtime.state()));
    const auto read = FunctionRef::fromStack(runtime.state());

    EXPECT_TRUE(read);
}

TEST(FunctionRef, FromStackFailsForOtherTypes) {
    const auto runtime = makeRuntime();
    lua_pushnumber(runtime.state(), 1.0);

    const auto read = FunctionRef::fromStack(runtime.state());

    ASSERT_FALSE(read);
    EXPECT_EQ(read.error().kind(), ErrorKind::kTypeMismatch);
}

TEST(FunctionRef, FromStackThrowsForNullState) {
    EXPECT_THROW(std::ignore = FunctionRef::fromStack(nullptr), rtype::luau::exceptions::InvalidState);
}

TEST(FunctionRef, CopiesShareTheFunction) {
    const auto runtime = makeRuntime();
    const auto function = makeFunctionRef(runtime, "return function(a) return a * 2 end");

    const FunctionRef copy{function};

    EXPECT_TRUE(copy.ref().isValid());
    const Function callable = Function::fromRef(copy, "copy");
    ValuePack args{};
    args.add({4.0});
    const auto results = callable.call(args);
    ASSERT_TRUE(results);
    EXPECT_DOUBLE_EQ(results->values()[0].as<double>(), 8.0);
}

TEST(FunctionRef, PushFailsForOtherState) {
    const auto runtime = makeRuntime();
    const auto other = makeRuntime();
    const auto function = makeFunctionRef(runtime, "return function() end");

    const auto pushed = function.push(other.state());

    ASSERT_FALSE(pushed);
    EXPECT_EQ(pushed.error().kind(), ErrorKind::kRuntime);
}

TEST(FunctionRef, MovedFromHandleThrows) {
    const auto runtime = makeRuntime();
    auto function = makeFunctionRef(runtime, "return function() end");
    const FunctionRef moved{std::move(function)};

    EXPECT_THROW(std::ignore = function.ref(), rtype::luau::exceptions::InvalidRef);
    EXPECT_THROW(std::ignore = function.push(runtime.state()), rtype::luau::exceptions::InvalidRef);
}

TEST(FunctionRef, ValueHoldsFunctionRef) {
    const auto runtime = makeRuntime();
    const auto function = makeFunctionRef(runtime, "return function() return 1 end");

    const Value value{function};

    EXPECT_TRUE(value.is<FunctionRef>());
    const Value copy{value};
    EXPECT_TRUE(copy.is<FunctionRef>());
}

TEST(FunctionRef, ValuePushesFunctionBack) {
    const auto runtime = makeRuntime();
    const Value value{makeFunctionRef(runtime, "return function() return 1 end")};

    ASSERT_TRUE(value.push(runtime.state()));

    EXPECT_TRUE(FunctionRef::fromStack(runtime.state()));
}
