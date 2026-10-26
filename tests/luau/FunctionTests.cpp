/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** FunctionTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/luau/CallContext.hpp"
#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/Ref.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
using CallContext = rtype::luau::CallContext;
using Function = rtype::luau::Function;
using Value = rtype::luau::Value;
using ValuePack = rtype::luau::ValuePack;

/// @brief More arguments than a Lua stack can hold (Luau caps it at 8000 slots).
constexpr std::size_t kTooManyArgs = 10000;

/// @brief Creates a runtime with the standard libraries, failing the test if it cannot be created.
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

/// @brief Builds arguments whose last one cannot be pushed, after @p validArgs pushable ones.
///
/// The last one is a table of @p foreign, which no other runtime can push; @p foreign must outlive the arguments.
ValuePack makeUnpushableArgs(const rtype::luau::Runtime& foreign, const std::size_t validArgs) {
    ValuePack args{};
    for (std::size_t i = 0; i < validArgs; ++i) {
        args.add({0.0});
    }
    args.add({rtype::luau::Table::create(foreign.state()).value()});
    return args;
}

constexpr std::string_view kForeignTableMessage = "the table belongs to another Lua state";
}  // namespace

TEST(Function, CallWithoutArgumentsPassesNoArguments) {
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
    ValuePack args{};
    args.add({std::string{"first"}});
    args.add({2.0});
    args.add({true});

    EXPECT_TRUE(function.call(args));
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
    ValuePack args{};
    args.add({Value::Nil{}});
    args.add({false});
    args.add({1.5});
    args.add({Value::Vector{1.0F, 2.0F, 3.0F}});
    args.add({std::string{"text"}});

    EXPECT_TRUE(function.call(args));
}

TEST(Function, CallPassesMoreArgumentsThanMinimumStack) {
    const auto runtime = makeRuntime();
    constexpr std::size_t kArgCount = 1000;
    const Function function = makeFunction(runtime, R"(
        assert(select("#", ...) == 1000)
        assert(select(1000, ...) == 999)
    )");
    ValuePack args{};
    for (std::size_t i = 0; i < kArgCount; ++i) {
        args.add({static_cast<double>(i)});
    }

    EXPECT_TRUE(function.call(args));
}

TEST(Function, CallFailsWhenFunctionFails) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, R"(local a = ...; assert(a == "expected"))");
    ValuePack args{};
    args.add({std::string{"unexpected"}});

    const auto result = function.call(args);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
}

TEST(Function, CallFailsWithTooManyArguments) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, "error('should not run')");
    ValuePack args{};
    for (std::size_t i = 0; i < kTooManyArgs; ++i) {
        args.add({0.0});
    }

    const auto result = function.call(args);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kStackOverflow);
    EXPECT_EQ(result.error().message(), "Lua stack overflow");
}

TEST(Function, CallFailsForTableArgument) {
    const auto runtime = makeRuntime();
    const auto foreign = makeRuntime();
    const Function function = makeFunction(runtime, "error('should not run')");

    const auto result = function.call(makeUnpushableArgs(foreign, 0));

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
    EXPECT_EQ(result.error().message(), kForeignTableMessage);
}

TEST(Function, CallWorksAfterFailedPush) {
    const auto runtime = makeRuntime();
    const auto foreign = makeRuntime();
    const Function function = makeFunction(runtime, R"(local a = ...; assert(a == "ok"))");
    EXPECT_FALSE(function.call(makeUnpushableArgs(foreign, 3)));
    ValuePack args{};
    args.add({std::string{"ok"}});

    EXPECT_TRUE(function.call(args));
}

TEST(Function, FailedPushDoesNotLeaveValuesOnStack) {
    const auto runtime = makeRuntime();
    const auto foreign = makeRuntime();
    const Function function = makeFunction(runtime, "error('should not run')");
    // Each failed call pushes the function and 100 arguments. If they were left on the stack, they would add
    // up past Luau's 8000-slot cap within 100 calls, and pushing the arguments would fail with a stack overflow
    // instead.
    constexpr int kCalls = 100;
    const ValuePack args = makeUnpushableArgs(foreign, 100);

    for (int i = 0; i < kCalls; ++i) {
        const auto result = function.call(args);
        ASSERT_FALSE(result) << "call " << i;
        ASSERT_EQ(result.error().message(), kForeignTableMessage) << "call " << i;
    }
}

TEST(Function, CallReturnsNoValues) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, "local _ = 1");

    const auto result = function.call();

    ASSERT_TRUE(result);
    EXPECT_TRUE(result->values().empty());
}

TEST(Function, CallReturnsValuesInOrder) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, R"(return "first", 2, true)");

    const auto result = function.call();

    ASSERT_TRUE(result);
    const auto values = result->values();
    ASSERT_EQ(values.size(), 3);
    EXPECT_EQ(values[0].as<std::string>(), "first");
    EXPECT_EQ(values[1].as<double>(), 2.0);
    EXPECT_EQ(values[2].as<bool>(), true);
}

TEST(Function, CallFailsForUnreadableReturnValue) {
    const auto runtime = makeRuntime();
    const Function function = makeFunction(runtime, "return 1, function() end, 3");

    const auto result = function.call();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kTypeMismatch);
}

TEST(Function, UnreadableReturnValueDoesNotLeaveValuesOnStack) {
    const auto runtime = makeRuntime();
    // Each call returns 100 numbers followed by a function. If they were left on the stack, they would add up
    // past Luau's 8000-slot cap within 100 calls, and the call would fail with a stack overflow instead.
    const Function function = makeFunction(runtime, R"(
        local values = table.create(100, 0)
        values[101] = function() end
        return table.unpack(values, 1, 101)
    )");
    constexpr int kCalls = 100;

    for (int i = 0; i < kCalls; ++i) {
        const auto result = function.call();
        ASSERT_FALSE(result) << "call " << i;
        ASSERT_EQ(result.error().kind(), rtype::luau::ErrorKind::kTypeMismatch) << "call " << i;
    }
}

TEST(Function, NativeReceivesArgumentsInOrder) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    std::vector<Value> received{};
    native.setHandler([&received](const CallContext& context) -> ValuePack {
        received.assign(context.args().begin(), context.args().end());
        return {};
    });
    const Function function = makeFunction(runtime, R"(native("first", 2, true, nil))");

    ASSERT_TRUE(function.call());

    ASSERT_EQ(received.size(), 4U);
    EXPECT_EQ(received[0].as<std::string>(), "first");
    EXPECT_EQ(received[1].as<double>(), 2.0);
    EXPECT_EQ(received[2].as<bool>(), true);
    EXPECT_TRUE(received[3].is<Value::Nil>());
}

TEST(Function, NativeReturnsValuesInOrder) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.setHandler([](const CallContext&) -> ValuePack {
        ValuePack result{};
        result.add({std::string{"first"}});
        result.add({2.0});
        result.add({Value::Vector{1.0F, 2.0F, 3.0F}});
        return result;
    });
    const Function function = makeFunction(runtime, R"(
        assert(select("#", native()) == 3)
        local a, b, v = native()
        assert(a == "first" and b == 2)
        assert(typeof(v) == "vector" and v.x == 1 and v.y == 2 and v.z == 3)
    )");

    EXPECT_TRUE(function.call());
}

TEST(Function, NativeWithoutValuesReturnsNothing) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.setHandler([](const CallContext&) -> ValuePack { return {}; });
    const Function function = makeFunction(runtime, R"(assert(select("#", native(1, 2)) == 0))");

    EXPECT_TRUE(function.call());
}

TEST(Function, NativeEchoesArgumentsBackToLuau) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.setHandler([](const CallContext& context) -> ValuePack {
        ValuePack result{};
        for (const Value& arg : context.args()) {
            result.add(arg);
        }
        return result;
    });
    const Function function = makeFunction(runtime, "return native(...)");
    ValuePack args{};
    args.add({std::string{"text"}});
    args.add({false});

    const auto result = function.call(args);

    ASSERT_TRUE(result);
    ASSERT_EQ(result->values().size(), 2U);
    EXPECT_EQ(result->values()[0].as<std::string>(), "text");
    EXPECT_EQ(result->values()[1].as<bool>(), false);
}

TEST(Function, NativeReturnsMoreValuesThanMinimumStack) {
    const auto runtime = makeRuntime();
    constexpr std::size_t kValueCount = 1000;
    Function native = Function::create(runtime.state(), "native");
    native.setHandler([](const CallContext&) -> ValuePack {
        ValuePack result{};
        for (std::size_t i = 0; i < kValueCount; ++i) {
            result.add({static_cast<double>(i)});
        }
        return result;
    });
    const Function function = makeFunction(runtime, R"(
        assert(select("#", native()) == 1000)
        assert(select(1000, native()) == 999)
    )");

    EXPECT_TRUE(function.call());
}

TEST(Function, NativeRaisesErrorForUnreadableArgument) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    bool called{false};
    native.setHandler([&called](const CallContext&) -> ValuePack {
        called = true;
        return {};
    });
    const Function function = makeFunction(runtime, R"(
        local ok, err = pcall(native, 1, function() end)
        assert(not ok)
        assert(string.find(err, "invalid argument to 'native' (value #2: ", 1, true), err)
    )");

    const auto result = function.call();

    EXPECT_TRUE(result) << result.error().message();
    EXPECT_FALSE(called);
}

TEST(Function, NativeRaisesErrorForUnpushableValue) {
    const auto runtime = makeRuntime();
    const auto foreign = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.setHandler([&foreign](const CallContext&) -> ValuePack {
        ValuePack result{};
        result.add({rtype::luau::Table::create(foreign.state()).value()});
        return result;
    });
    const Function function = makeFunction(runtime, R"(
        local ok, err = pcall(native)
        assert(not ok)
        assert(err == "native function 'native': the table belongs to another Lua state", err)
    )");

    const auto result = function.call();

    EXPECT_TRUE(result) << result.error().message();
}

TEST(Function, NativeRaisesErrorWhenHandlerThrowsModuleException) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.setHandler([](const CallContext&) -> ValuePack { throw rtype::luau::exceptions::InvalidOperation{"boom"}; });
    const Function function = makeFunction(runtime, R"(
        local ok, err = pcall(native)
        assert(not ok)
        assert(err == "native function 'native': boom", err)
    )");

    const auto result = function.call();

    EXPECT_TRUE(result) << result.error().message();
}

TEST(Function, NativeRaisesErrorWhenHandlerThrowsStdException) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.setHandler([](const CallContext&) -> ValuePack { throw std::runtime_error{"oops"}; });
    const Function function = makeFunction(runtime, "native()");

    const auto result = function.call();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
    EXPECT_EQ(result.error().message(), "oops");
}

TEST(Function, NativeRaisesErrorWhenHandlerThrowsNonStdException) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.setHandler([](const CallContext&) -> ValuePack { throw 42; });
    const Function function = makeFunction(runtime, "native()");

    const auto result = function.call();

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
    EXPECT_NE(result.error().message().find("unknown exception"), std::string::npos);
}

TEST(Function, NativeHandlerCanCallBackIntoLuau) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    // The handler calls back into Luau: the nested call runs in protected mode, so its error comes back as a
    // Result and the native call itself succeeds.
    const Function failing = makeFunction(runtime, "error('nested', 0)");
    native.setHandler([&failing](const CallContext&) -> ValuePack {
        const auto nested = failing.call();
        ValuePack result{};
        result.add({std::string{nested ? "ok" : nested.error().message()}});
        return result;
    });
    const Function function = makeFunction(runtime, R"(assert(native() == "nested"))");

    const auto result = function.call();

    EXPECT_TRUE(result) << result.error().message();
}

TEST(Function, NativeCallsDoNotLeaveValuesOnStack) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.setHandler([](const CallContext& context) -> ValuePack {
        ValuePack result{};
        for (const Value& arg : context.args()) {
            result.add(arg);
        }
        return result;
    });
    // Each call passes and returns 100 values, 100 times. If they were left on the stack, they would add up past
    // Luau's 8000-slot cap, and the calls would fail with a stack overflow instead.
    const Function function = makeFunction(runtime, R"(
        local values = table.create(100, 0)
        for _ = 1, 100 do
            assert(select("#", native(table.unpack(values))) == 100)
        end
    )");
    constexpr int kCalls = 10;

    for (int i = 0; i < kCalls; ++i) {
        const auto result = function.call();
        ASSERT_TRUE(result) << "call " << i << ": " << result.error().message();
    }
}

/// @brief Runs a chunk in which the global `native` was bound with `bind`, failing the test if the chunk fails.
void expectChunkSucceeds(const rtype::luau::Runtime& runtime, const std::string& source) {
    const auto result = makeFunction(runtime, source).call();
    EXPECT_TRUE(result) << result.error().message();
}

TEST(FunctionBind, ReadsTypedArgumentsInOrder) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    std::string name{};
    double number{};
    bool flag{};
    native.bind([&](const std::string& s, const double d, const bool b) {
        name = s;
        number = d;
        flag = b;
        return ValuePack{};
    });

    expectChunkSucceeds(runtime, R"(native("first", 2, true))");

    EXPECT_EQ(name, "first");
    EXPECT_EQ(number, 2.0);
    EXPECT_TRUE(flag);
}

TEST(FunctionBind, ReadsVectorTableAndNilArguments) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    Value::Vector vector{};
    bool tableRead = false;
    bool nilRead = false;
    native.bind([&](const Value::Vector& v, const rtype::luau::Table& t, const Value::Nil&) {
        vector = v;
        tableRead = t.size().value_or(1) == 0;
        nilRead = true;
        return ValuePack{};
    });

    expectChunkSucceeds(runtime, "native(vector.create(1, 2, 3), {}, nil)");

    EXPECT_EQ(vector, (Value::Vector{1.0F, 2.0F, 3.0F}));
    EXPECT_TRUE(tableRead);
    EXPECT_TRUE(nilRead);
}

TEST(FunctionBind, ReturnsValuesToLua) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.bind([](const double a, const double b) {
        ValuePack result{};
        result.add({a + b});
        result.add({std::string{"sum"}});
        return result;
    });

    expectChunkSucceeds(runtime, R"(
        local sum, label = native(1, 2)
        assert(sum == 3 and label == "sum")
    )");
}

TEST(FunctionBind, AcceptsHandlerWithoutParameters) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    int calls = 0;
    native.bind([&calls]() {
        ++calls;
        return ValuePack{};
    });

    expectChunkSucceeds(runtime, R"(
        native()
        assert(not pcall(native, 1))
    )");

    EXPECT_EQ(calls, 1);
}

TEST(FunctionBind, AcceptsFunctionPointer) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.bind(+[](const double d) {
        ValuePack result{};
        result.add({d * 2});
        return result;
    });

    expectChunkSucceeds(runtime, "assert(native(4) == 8)");
}

TEST(FunctionBind, RaisesErrorForWrongArgumentCountWithoutRunningHandler) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    int calls = 0;
    native.bind([&calls](const double, const double) {
        ++calls;
        return ValuePack{};
    });

    expectChunkSucceeds(runtime, R"(
        local ok, err = pcall(native, 1)
        assert(not ok and err == "native function 'native': expected 2 arguments, got 1", err)
        ok, err = pcall(native, 1, 2, 3)
        assert(not ok and err == "native function 'native': expected 2 arguments, got 3", err)
    )");

    EXPECT_EQ(calls, 0);
}

TEST(FunctionBind, RaisesErrorNamingArgumentAndTypesForWrongType) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    int calls = 0;
    native.bind([&calls](const double, const double) {
        ++calls;
        return ValuePack{};
    });

    expectChunkSucceeds(runtime, R"(
        local ok, err = pcall(native, 1, "x")
        assert(not ok and err == "native function 'native': argument 2: expected number, got string", err)
        ok, err = pcall(native, nil, 1)
        assert(not ok and err == "native function 'native': argument 1: expected number, got nil", err)
        ok, err = pcall(native, {}, 1)
        assert(not ok and err == "native function 'native': argument 1: expected number, got table", err)
    )");

    EXPECT_EQ(calls, 0);
}

TEST(FunctionBind, OptionalTrailingArgumentsMayBeOmittedOrNil) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    std::vector<std::string> calls{};
    native.bind([&calls](const double, const std::optional<std::string>& label, const std::optional<double> extra) {
        calls.push_back(std::format("{}/{}", label.value_or("-"), extra ? "x" : "-"));
        return ValuePack{};
    });

    expectChunkSucceeds(runtime, R"(
        native(1)
        native(1, "a")
        native(1, nil, 2)
        native(1, "b", 2)
        local ok, err = pcall(native, 1, 2)
        assert(not ok and err == "native function 'native': argument 2: expected string, got number", err)
        ok, err = pcall(native)
        assert(not ok and err == "native function 'native': expected 1 to 3 arguments, got 0", err)
        ok, err = pcall(native, 1, "a", 2, 3)
        assert(not ok and err == "native function 'native': expected 1 to 3 arguments, got 4", err)
    )");

    EXPECT_EQ(calls, (std::vector<std::string>{"-/-", "a/-", "-/x", "b/x"}));
}

TEST(FunctionBind, CallsDoNotLeaveValuesOnStack) {
    const auto runtime = makeRuntime();
    Function native = Function::create(runtime.state(), "native");
    native.bind([](const double d) {
        ValuePack result{};
        result.add({d});
        return result;
    });

    // 10000 successful and failing calls would overflow Luau's 8000-slot stack if any value were left behind.
    expectChunkSucceeds(runtime, R"(
        for i = 1, 10000 do
            assert(native(i) == i)
            assert(not pcall(native, "x"))
        end
    )");
}
