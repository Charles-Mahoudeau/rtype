/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** StoredFunctionTests
*/

#include <gtest/gtest.h>
#include <lua.h>
#include <lualib.h>

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
using Function = rtype::luau::Function;
using Userdata = rtype::luau::Userdata;
using Value = rtype::luau::Value;
using ValuePack = rtype::luau::ValuePack;

/// @brief A C++ type a stored function receives, as a system receives a component view.
struct Counter {
    double value{0};
};

/// @brief Creates a runtime with the standard libraries, failing the test if it cannot be created.
rtype::luau::Runtime makeRuntime() {
    auto runtime = rtype::luau::Runtime::create({
        .libs = rtype::luau::RuntimeConfig::Libs::kStandard,
        .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kStandard,
    });
    EXPECT_TRUE(runtime);
    return std::move(*runtime);
}

/// @brief Registers `Counter` with a read-write `value`.
rtype::luau::Usertype<Counter> registerCounter(rtype::luau::Runtime& runtime) {
    auto registered = runtime.usertype<Counter>("Counter");
    EXPECT_TRUE(registered);
    return registered->getter("value", [](const Counter& counter) { return counter.value; })
        .setter("value", [](Counter& counter, const double value) { counter.value = value; });
}

/// @brief Runs @p source as the script @p name, failing the test if it raises an error.
void run(const rtype::luau::Runtime& runtime, const std::string& name, const std::string& source) {
    const auto script = runtime.load(name, source);
    ASSERT_TRUE(script);
    const auto result = runtime.run(*script);
    ASSERT_TRUE(result) << result.error().message();
}

/// @brief Gets the function `run` of the global table `system`, as a system declaration holds it.
Function storedRun(const rtype::luau::Runtime& runtime) {
    const auto system = runtime.globals().get("system");
    EXPECT_TRUE(system);
    auto function = system->as<rtype::luau::Table>().function("run");
    EXPECT_TRUE(function);
    return std::move(*function);
}
}  // namespace

TEST(StoredFunction, IsCalledWithAUserdataAndANumber) {
    auto runtime = makeRuntime();
    const auto usertype = registerCounter(runtime);
    run(runtime, "systems.luau", R"(
        system = { run = function(counter, dt) counter.value = counter.value + dt end }
    )");
    const Function stored = storedRun(runtime);
    const auto userdata = usertype.create(Counter{.value = 1});
    ASSERT_TRUE(userdata);

    ValuePack args{};
    args.add({*userdata});
    args.add({0.5});
    ASSERT_TRUE(stored.call(args));

    EXPECT_EQ(userdata->tryAs<Counter>()->value, 1.5);
}

TEST(StoredFunction, KeepsWorkingAcrossFramesAndCollections) {
    auto runtime = makeRuntime();
    const auto usertype = registerCounter(runtime);
    run(runtime, "systems.luau", R"(
        local calls = 0
        system = { run = function(counter, dt) calls = calls + 1 counter.value = calls end }
    )");
    const Function stored = storedRun(runtime);
    const auto userdata = usertype.create(Counter{});
    ASSERT_TRUE(userdata);

    for (int frame = 1; frame <= 100; ++frame) {
        lua_gc(runtime.state(), LUA_GCCOLLECT, 0);
        ValuePack args{};
        args.add({*userdata});
        args.add({0.0});
        ASSERT_TRUE(stored.call(args)) << "frame " << frame;
        ASSERT_EQ(userdata->tryAs<Counter>()->value, frame);
    }
}

TEST(StoredFunction, ReportsTheScriptNameAndTheLineOfAnError) {
    auto runtime = makeRuntime();
    const auto usertype = registerCounter(runtime);
    run(runtime, "Shield.luau", "system = {\n    run = function(counter, dt)\n        error(\"boom\")\n    end,\n}\n");
    const Function stored = storedRun(runtime);
    const auto userdata = usertype.create(Counter{});
    ASSERT_TRUE(userdata);

    ValuePack args{};
    args.add({*userdata});
    args.add({0.0});
    const auto result = stored.call(args);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
    EXPECT_EQ(result.error().message(), "Shield.luau:3: boom");
}

TEST(StoredFunction, ReportsTheLineOfAnErrorRaisedByAMethod) {
    auto runtime = makeRuntime();
    const auto usertype = registerCounter(runtime);
    run(runtime, "Shield.luau", "system = { run = function(counter, dt)\n    counter.value = \"x\"\nend }");
    const Function stored = storedRun(runtime);
    const auto userdata = usertype.create(Counter{});
    ASSERT_TRUE(userdata);

    ValuePack args{};
    args.add({*userdata});
    args.add({0.0});
    const auto result = stored.call(args);

    ASSERT_FALSE(result);
    EXPECT_NE(result.error().message().find("Shield.luau:2:"), std::string_view::npos) << result.error().message();
}

TEST(StoredFunction, IteratesOverANativeIterator) {
    auto runtime = makeRuntime();
    auto entities = std::make_shared<std::vector<double>>(std::vector<double>{10, 20, 30});
    auto next = Function::create(runtime.state(), "query");
    next.setHandler([entities, index = std::size_t{0}](const rtype::luau::CallContext&) mutable -> ValuePack {
        ValuePack results{};
        if (index < entities->size()) {
            results.add({(*entities)[index++]});
        }
        return results;
    });
    ASSERT_TRUE(runtime.globals().set("query", Value{next.ref()}));

    run(runtime, "loop.luau", R"(
        sum = 0
        for value in query do
            sum = sum + value
        end
    )");

    const auto sum = runtime.globals().get("sum");
    ASSERT_TRUE(sum);
    EXPECT_EQ(sum->as<double>(), 60);
}
