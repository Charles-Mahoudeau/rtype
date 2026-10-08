/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ValueTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "rtype/luau/Value.hpp"

namespace {
using Value = rtype::luau::Value;
}  // namespace

TEST(Value, HoldsNil) {
    const Value value{Value::Nil{}};

    EXPECT_TRUE(value.is<Value::Nil>());
    EXPECT_FALSE(value.is<bool>());
    EXPECT_FALSE(value.isNumeric());
}

TEST(Value, HoldsBool) {
    const Value value{true};

    EXPECT_TRUE(value.is<bool>());
    EXPECT_TRUE(value.as<bool>());
    EXPECT_FALSE(value.is<double>());
}

TEST(Value, HoldsDouble) {
    const Value value{1.5};

    EXPECT_TRUE(value.is<double>());
    EXPECT_FALSE(value.is<std::int64_t>());
    EXPECT_DOUBLE_EQ(value.as<double>(), 1.5);
}

TEST(Value, HoldsInt64) {
    const Value value{std::int64_t{42}};

    EXPECT_TRUE(value.is<std::int64_t>());
    EXPECT_FALSE(value.is<double>());
    EXPECT_EQ(value.as<std::int64_t>(), 42);
}

TEST(Value, HoldsVector) {
    const Value::Vector vector{1.0F, 2.0F, 3.0F};
    const Value value{vector};

    ASSERT_TRUE(value.is<Value::Vector>());
    EXPECT_EQ(value.as<Value::Vector>(), vector);
}

TEST(Value, HoldsString) {
    const Value value{std::string{"hello"}};

    ASSERT_TRUE(value.is<std::string>());
    EXPECT_EQ(value.as<std::string>(), "hello");
}

TEST(Value, ConstAsThrowsOnWrongType) {
    const Value value{true};

    EXPECT_THROW(std::ignore = value.as<double>(), std::bad_variant_access);
    EXPECT_THROW(std::ignore = value.as<std::string>(), std::bad_variant_access);
}

TEST(Value, MutableAsThrowsOnWrongType) {
    Value value{true};

    EXPECT_THROW(std::ignore = value.as<double>(), std::bad_variant_access);
}

TEST(Value, MutableAsAllowsModification) {
    Value value{std::string{"abc"}};

    value.as<std::string>() += "def";

    EXPECT_EQ(value.as<std::string>(), "abcdef");
}

TEST(Value, TryAsReturnsPointerOnMatch) {
    const Value value{2.5};

    const auto* ptr = value.tryAs<double>();

    ASSERT_NE(ptr, nullptr);
    EXPECT_DOUBLE_EQ(*ptr, 2.5);
    EXPECT_EQ(ptr, &value.as<double>());
}

TEST(Value, TryAsReturnsNullptrOnMismatch) {
    const Value value{2.5};

    EXPECT_EQ(value.tryAs<bool>(), nullptr);
    EXPECT_EQ(value.tryAs<std::string>(), nullptr);
}

TEST(Value, TryAsIsNoexcept) {
    const Value value{Value::Nil{}};

    static_assert(noexcept(value.tryAs<bool>()));
    EXPECT_EQ(value.tryAs<bool>(), nullptr);
}

TEST(Value, IsNumericOnlyForDoubleAndInt64) {
    EXPECT_TRUE(Value{1.0}.isNumeric());
    EXPECT_TRUE(Value{std::int64_t{1}}.isNumeric());
    EXPECT_FALSE(Value{Value::Nil{}}.isNumeric());
    EXPECT_FALSE(Value{true}.isNumeric());
    EXPECT_FALSE(Value{std::string{"1"}}.isNumeric());
    EXPECT_FALSE(Value{Value::Vector{}}.isNumeric());
}

TEST(Value, ToInt32FromInt64) {
    EXPECT_EQ(Value{std::int64_t{123}}.toInt32(), 123);
    EXPECT_EQ(Value{std::int64_t{-123}}.toInt32(), -123);
}

TEST(Value, ToInt32FromDoubleTruncatesTowardsZero) {
    EXPECT_EQ(Value{3.9}.toInt32(), 3);
    EXPECT_EQ(Value{-3.9}.toInt32(), -3);
}

TEST(Value, ToInt32AcceptsLimits) {
    constexpr auto kMin = std::numeric_limits<std::int32_t>::min();
    constexpr auto kMax = std::numeric_limits<std::int32_t>::max();

    EXPECT_EQ(Value{std::int64_t{kMin}}.toInt32(), kMin);
    EXPECT_EQ(Value{std::int64_t{kMax}}.toInt32(), kMax);
    EXPECT_EQ(Value{static_cast<double>(kMin)}.toInt32(), kMin);
    EXPECT_EQ(Value{static_cast<double>(kMax)}.toInt32(), kMax);
}

TEST(Value, ToInt32RejectsOutOfRange) {
    constexpr auto kMin = std::numeric_limits<std::int32_t>::min();
    constexpr auto kMax = std::numeric_limits<std::int32_t>::max();

    EXPECT_EQ(Value{std::int64_t{kMax} + 1}.toInt32(), std::nullopt);
    EXPECT_EQ(Value{std::int64_t{kMin} - 1}.toInt32(), std::nullopt);
    EXPECT_EQ(Value{static_cast<double>(kMax) + 1.0}.toInt32(), std::nullopt);
    EXPECT_EQ(Value{static_cast<double>(kMin) - 1.0}.toInt32(), std::nullopt);
}

TEST(Value, ToInt32RejectsNonNumeric) {
    EXPECT_EQ(Value{Value::Nil{}}.toInt32(), std::nullopt);
    EXPECT_EQ(Value{true}.toInt32(), std::nullopt);
    EXPECT_EQ(Value{std::string{"1"}}.toInt32(), std::nullopt);
    EXPECT_EQ(Value{Value::Vector{}}.toInt32(), std::nullopt);
}

TEST(Value, ToUInt32FromInt64) {
    EXPECT_EQ(Value{std::int64_t{0}}.toUInt32(), 0U);
    EXPECT_EQ(Value{std::int64_t{123}}.toUInt32(), 123U);
}

TEST(Value, ToUInt32FromDoubleTruncatesTowardsZero) { EXPECT_EQ(Value{3.9}.toUInt32(), 3U); }

TEST(Value, ToUInt32AcceptsValuesAboveInt32Max) {
    constexpr auto kMax = std::numeric_limits<std::uint32_t>::max();

    EXPECT_EQ(Value{std::int64_t{kMax}}.toUInt32(), kMax);
    EXPECT_EQ(Value{static_cast<double>(kMax)}.toUInt32(), kMax);
    EXPECT_EQ(Value{std::int64_t{3'000'000'000}}.toUInt32(), 3'000'000'000U);
}

TEST(Value, ToUInt32RejectsOutOfRange) {
    constexpr auto kMax = std::numeric_limits<std::uint32_t>::max();

    EXPECT_EQ(Value{std::int64_t{-1}}.toUInt32(), std::nullopt);
    EXPECT_EQ(Value{-1.0}.toUInt32(), std::nullopt);
    EXPECT_EQ(Value{std::int64_t{kMax} + 1}.toUInt32(), std::nullopt);
    EXPECT_EQ(Value{static_cast<double>(kMax) + 1.0}.toUInt32(), std::nullopt);
}

TEST(Value, ToUInt32RejectsNonNumeric) {
    EXPECT_EQ(Value{Value::Nil{}}.toUInt32(), std::nullopt);
    EXPECT_EQ(Value{false}.toUInt32(), std::nullopt);
    EXPECT_EQ(Value{std::string{"1"}}.toUInt32(), std::nullopt);
}

TEST(Value, CopyIsIndependent) {
    const Value original{std::string{"abc"}};
    Value copy{original};

    copy.as<std::string>() = "xyz";

    EXPECT_EQ(original.as<std::string>(), "abc");
    EXPECT_EQ(copy.as<std::string>(), "xyz");
}

TEST(Value, CopyAssignmentReplacesHeldType) {
    const Value source{std::string{"abc"}};
    Value target{1.0};

    target = source;

    ASSERT_TRUE(target.is<std::string>());
    EXPECT_EQ(target.as<std::string>(), "abc");
}

TEST(Value, MoveConstructionTransfersContent) {
    Value source{std::string{"a string long enough to defeat the small string optimisation"}};

    const Value moved{std::move(source)};

    ASSERT_TRUE(moved.is<std::string>());
    EXPECT_EQ(moved.as<std::string>(), "a string long enough to defeat the small string optimisation");
}

TEST(Value, MoveAssignmentTransfersContent) {
    Value source{std::string{"moved"}};
    Value target{Value::Nil{}};

    target = std::move(source);

    ASSERT_TRUE(target.is<std::string>());
    EXPECT_EQ(target.as<std::string>(), "moved");
}

TEST(Value, SpecialMembersAreNoexcept) {
    static_assert(std::is_nothrow_copy_constructible_v<Value>);
    static_assert(std::is_nothrow_copy_assignable_v<Value>);
    static_assert(std::is_nothrow_move_constructible_v<Value>);
    static_assert(std::is_nothrow_move_assignable_v<Value>);
    static_assert(std::is_nothrow_destructible_v<Value>);
    SUCCEED();
}
