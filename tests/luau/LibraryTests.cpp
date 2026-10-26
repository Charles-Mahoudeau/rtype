/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** LibraryTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "rtype/luau/Function.hpp"
#include "rtype/luau/Library.hpp"
#include "rtype/luau/LibraryBuilder.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
using Function = rtype::luau::Function;
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

/// @brief Binds @p function to a native function that returns its argument multiplied by @p factor.
void bindMultiplier(Function& function, const double factor) {
    function.bind([factor](const double value) {
        ValuePack results{};
        results.add({value * factor});
        return results;
    });
}

/// @brief Runs @p source, which must assign a number to the global `result`, and returns that number.
double runForResult(const rtype::luau::Runtime& runtime, const std::string& source) {
    const auto script = runtime.load("library-test", source);
    EXPECT_TRUE(script);
    EXPECT_TRUE(runtime.run(*script));
    const auto result = runtime.globals().get("result");
    EXPECT_TRUE(result);
    const auto* number = result->tryAs<double>();
    EXPECT_NE(number, nullptr);
    return *number;
}
}  // namespace

TEST(Library, KeepsItsName) {
    auto runtime = makeRuntime();
    rtype::luau::LibraryBuilder builder{runtime};
    builder.setName("mathx");

    const auto library = builder.build();

    EXPECT_EQ(library.name(), "mathx");
}

TEST(Library, StoresItsNameInTheTable) {
    auto runtime = makeRuntime();
    rtype::luau::LibraryBuilder builder{runtime};
    builder.setName("mathx");

    const auto library = builder.build();

    const auto name = library.table().get("__name");
    ASSERT_TRUE(name);
    const auto* text = name->tryAs<std::string>();
    ASSERT_NE(text, nullptr);
    EXPECT_EQ(*text, "mathx");
}

TEST(Library, RegistersFunctionsUnderTheirName) {
    auto runtime = makeRuntime();
    rtype::luau::LibraryBuilder builder{runtime};
    builder.setName("mathx");
    bindMultiplier(builder.function("double"), 2.0);
    bindMultiplier(builder.function("triple"), 3.0);

    const auto library = builder.build();

    ASSERT_TRUE(runtime.globals().set("mathx", Value{library.table()}));
    EXPECT_EQ(runForResult(runtime, "result = mathx.double(4) + mathx.triple(4)"), 20.0);
}

TEST(Library, FunctionsOutliveTheBuilder) {
    auto runtime = makeRuntime();
    auto builder = std::make_unique<rtype::luau::LibraryBuilder>(runtime);
    builder->setName("mathx");
    bindMultiplier(builder->function("double"), 2.0);
    const auto library = builder->build();
    builder.reset();

    ASSERT_TRUE(runtime.globals().set("mathx", Value{library.table()}));
    EXPECT_EQ(runForResult(runtime, "result = mathx.double(21)"), 42.0);
}

TEST(Library, FunctionReferencesStayValidWhileAddingMore) {
    constexpr std::size_t kFunctionCount = 200;
    auto runtime = makeRuntime();
    rtype::luau::LibraryBuilder builder{runtime};
    builder.setName("many");

    std::vector<Function*> functions;
    for (std::size_t i = 0; i < kFunctionCount; ++i) {
        functions.push_back(&builder.function("f" + std::to_string(i)));
    }
    for (std::size_t i = 0; i < kFunctionCount; ++i) {
        bindMultiplier(*functions[i], static_cast<double>(i));
    }

    const auto library = builder.build();

    ASSERT_TRUE(runtime.globals().set("many", Value{library.table()}));
    EXPECT_EQ(runForResult(runtime, "result = many.f0(5) + many.f1(5) + many.f199(5)"), 1000.0);
}
