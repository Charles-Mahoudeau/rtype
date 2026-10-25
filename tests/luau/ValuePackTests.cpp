/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ValuePackTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <string>
#include <utility>

#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
using Value = rtype::luau::Value;
using ValuePack = rtype::luau::ValuePack;

/// @brief More values than a Lua stack can hold (Luau caps it at 8000 slots).
constexpr std::size_t kTooManyValues = 10000;

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

TEST(ValuePack, IsEmptyByDefault) {
    const ValuePack pack{};

    EXPECT_EQ(pack.size(), 0U);
    EXPECT_TRUE(pack.values().empty());
}

TEST(ValuePack, AddKeepsValuesInOrder) {
    ValuePack pack{};

    pack.add({1.0});
    pack.add({std::string{"two"}});

    ASSERT_EQ(pack.size(), 2U);
    EXPECT_EQ(pack.values()[0].as<double>(), 1.0);
    EXPECT_EQ(pack.values()[1].as<std::string>(), "two");
}

TEST(ValuePack, CopyKeepsValues) {
    ValuePack pack{};
    pack.add({true});

    const ValuePack copy{pack};

    ASSERT_EQ(copy.size(), 1U);
    EXPECT_EQ(copy.values()[0].as<bool>(), true);
    EXPECT_EQ(pack.size(), 1U);
}

TEST(ValuePack, MoveKeepsValues) {
    ValuePack pack{};
    pack.add({true});

    const ValuePack moved{std::move(pack)};

    EXPECT_EQ(moved.size(), 1U);
}

TEST(ValuePack, PushReturnsValueCount) {
    const auto runtime = makeRuntime();
    ValuePack pack{};
    pack.add({Value::Nil{}});
    pack.add({false});
    pack.add({3.0});

    EXPECT_EQ(pack.push(runtime.state()).value(), 3U);
}

TEST(ValuePack, PushWithoutValuesReturnsZero) {
    const auto runtime = makeRuntime();
    const ValuePack pack{};

    EXPECT_EQ(pack.push(runtime.state()).value(), 0U);
}

TEST(ValuePack, PushGrowsStackBeyondMinimum) {
    const auto runtime = makeRuntime();
    ValuePack pack{};
    // Well above LUA_MINSTACK (20): pushing them without growing the stack would overflow it.
    constexpr std::size_t kValueCount = 1000;
    for (std::size_t i = 0; i < kValueCount; ++i) {
        pack.add({static_cast<double>(i)});
    }

    EXPECT_EQ(pack.push(runtime.state()).value(), kValueCount);
}

TEST(ValuePack, PushFailsWithStackOverflowWhenStackCannotGrow) {
    const auto runtime = makeRuntime();
    ValuePack pack{};
    for (std::size_t i = 0; i < kTooManyValues; ++i) {
        pack.add({0.0});
    }

    const auto pushed = pack.push(runtime.state());

    ASSERT_FALSE(pushed);
    EXPECT_EQ(pushed.error().kind(), rtype::luau::ErrorKind::kStackOverflow);
}

TEST(ValuePack, PushFailsForTableOfAnotherState) {
    const auto runtime = makeRuntime();
    const auto foreign = makeRuntime();
    ValuePack pack{};
    pack.add({rtype::luau::Table::create(foreign.state()).value()});

    const auto pushed = pack.push(runtime.state());

    ASSERT_FALSE(pushed);
    EXPECT_EQ(pushed.error().kind(), rtype::luau::ErrorKind::kRuntime);
}

TEST(ValuePack, FromStackReadsRangeInOrder) {
    const auto runtime = makeRuntime();
    ValuePack pushed{};
    pushed.add({std::string{"first"}});
    pushed.add({2.0});
    pushed.add({true});
    ASSERT_EQ(pushed.push(runtime.state()).value(), 3U);

    const auto pack = ValuePack::fromStack(runtime.state(), 2, 2);

    ASSERT_TRUE(pack);
    ASSERT_EQ(pack->size(), 2U);
    EXPECT_EQ(pack->values()[0].as<double>(), 2.0);
    EXPECT_EQ(pack->values()[1].as<bool>(), true);
}

TEST(ValuePack, FromStackDoesNotPop) {
    const auto runtime = makeRuntime();
    ValuePack pushed{};
    pushed.add({1.0});
    ASSERT_EQ(pushed.push(runtime.state()).value(), 1U);

    ASSERT_TRUE(ValuePack::fromStack(runtime.state(), 1, 1));
    const auto pack = ValuePack::fromStack(runtime.state(), 1, 1);

    ASSERT_TRUE(pack);
    ASSERT_EQ(pack->size(), 1U);
    EXPECT_EQ(pack->values()[0].as<double>(), 1.0);
}

TEST(ValuePack, FromStackWithZeroCountIsEmpty) {
    const auto runtime = makeRuntime();

    const auto pack = ValuePack::fromStack(runtime.state(), 1, 0);

    ASSERT_TRUE(pack);
    EXPECT_EQ(pack->size(), 0U);
}

TEST(ValuePack, FromStackFailsWithPositionOfUnreadableValue) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("chunk", "local _ = 1");
    ASSERT_TRUE(script);
    ValuePack pushed{};
    pushed.add({1.0});
    ASSERT_EQ(pushed.push(runtime.state()).value(), 1U);
    // A function cannot be read yet.
    script->closure().push(runtime.state());

    const auto pack = ValuePack::fromStack(runtime.state(), 1, 2);

    ASSERT_FALSE(pack);
    EXPECT_EQ(pack.error().kind(), rtype::luau::ErrorKind::kTypeMismatch);
    EXPECT_TRUE(pack.error().message().starts_with("value #2: ")) << pack.error().message();
}

TEST(ValuePack, PopReadsTopValuesInOrderAndPopsThem) {
    const auto runtime = makeRuntime();
    ValuePack pushed{};
    pushed.add({std::string{"bottom"}});
    pushed.add({1.0});
    pushed.add({2.0});
    ASSERT_EQ(pushed.push(runtime.state()).value(), 3U);

    const auto pack = ValuePack::pop(runtime.state(), 2);

    ASSERT_TRUE(pack);
    ASSERT_EQ(pack->size(), 2U);
    EXPECT_EQ(pack->values()[0].as<double>(), 1.0);
    EXPECT_EQ(pack->values()[1].as<double>(), 2.0);
    // Only the bottom value is left.
    const auto rest = ValuePack::pop(runtime.state(), 1);
    ASSERT_TRUE(rest);
    ASSERT_EQ(rest->size(), 1U);
    EXPECT_EQ(rest->values()[0].as<std::string>(), "bottom");
}

TEST(ValuePack, PopWithZeroCountIsEmpty) {
    const auto runtime = makeRuntime();

    const auto pack = ValuePack::pop(runtime.state(), 0);

    ASSERT_TRUE(pack);
    EXPECT_EQ(pack->size(), 0U);
}

TEST(ValuePack, PopLeavesValuesOnFailure) {
    const auto runtime = makeRuntime();
    const auto script = runtime.load("chunk", "local _ = 1");
    ASSERT_TRUE(script);
    ValuePack pushed{};
    pushed.add({1.0});
    ASSERT_EQ(pushed.push(runtime.state()).value(), 1U);
    // A function cannot be read yet.
    script->closure().push(runtime.state());

    const auto pack = ValuePack::pop(runtime.state(), 2);

    ASSERT_FALSE(pack);
    EXPECT_EQ(pack.error().kind(), rtype::luau::ErrorKind::kTypeMismatch);
    // Both values are still there: the number at the bottom, the function on top.
    const auto bottom = ValuePack::fromStack(runtime.state(), 1, 1);
    ASSERT_TRUE(bottom);
    EXPECT_EQ(bottom->values()[0].as<double>(), 1.0);
    EXPECT_FALSE(ValuePack::fromStack(runtime.state(), 2, 1));
}
