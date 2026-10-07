/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TableTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "rtype/luau/Table.hpp"
#include "rtype/luau/Value.hpp"

namespace {
using Table = rtype::luau::Table;
using Value = rtype::luau::Value;
}  // namespace

TEST(Table, IsEmptyByDefault) {
    const Table table;

    EXPECT_EQ(table.size(), 0U);
    EXPECT_EQ(table.begin(), table.end());
    EXPECT_EQ(table.cbegin(), table.cend());
}

TEST(Table, SetInsertsValue) {
    Table table;

    table.set("answer", Value{std::int64_t{42}});

    EXPECT_EQ(table.size(), 1U);
    const Value* value = table.tryGet("answer");
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(value->as<std::int64_t>(), 42);
}

TEST(Table, SetReturnsReferenceToStoredValue) {
    Table table;

    Value& stored = table.set("name", Value{std::string{"r-type"}});

    EXPECT_EQ(stored.as<std::string>(), "r-type");
    EXPECT_EQ(&stored, table.tryGet("name"));
}

TEST(Table, SetOverwritesExistingKey) {
    Table table;
    table.set("key", Value{1.0});

    table.set("key", Value{std::string{"replaced"}});

    EXPECT_EQ(table.size(), 1U);
    const Value* value = table.tryGet("key");
    ASSERT_NE(value, nullptr);
    EXPECT_TRUE(value->is<std::string>());
    EXPECT_EQ(value->as<std::string>(), "replaced");
}

TEST(Table, SetStoresMultipleKeys) {
    Table table;

    table.set("a", Value{true});
    table.set("b", Value{2.5});
    table.set("c", Value{Value::Nil{}});

    EXPECT_EQ(table.size(), 3U);
    EXPECT_TRUE(table.tryGet("a")->is<bool>());
    EXPECT_TRUE(table.tryGet("b")->is<double>());
    EXPECT_TRUE(table.tryGet("c")->is<Value::Nil>());
}

TEST(Table, TryGetReturnsNullptrForMissingKey) {
    Table table;
    table.set("present", Value{true});

    EXPECT_EQ(table.tryGet("absent"), nullptr);
}

TEST(Table, TryGetOnEmptyTableReturnsNullptr) {
    const Table table;

    EXPECT_EQ(table.tryGet("anything"), nullptr);
}

TEST(Table, TryGetIsCaseSensitive) {
    Table table;
    table.set("Key", Value{true});

    EXPECT_EQ(table.tryGet("key"), nullptr);
    EXPECT_NE(table.tryGet("Key"), nullptr);
}

TEST(Table, TryGetAcceptsStringView) {
    Table table;
    table.set("hello world", Value{1.0});
    const std::string_view full{"hello world"};

    EXPECT_NE(table.tryGet(full), nullptr);
    EXPECT_NE(table.tryGet(full.substr(0, 11)), nullptr);
    EXPECT_EQ(table.tryGet(full.substr(0, 5)), nullptr);
}

TEST(Table, TryGetAcceptsEmptyKey) {
    Table table;

    EXPECT_EQ(table.tryGet(""), nullptr);
    table.set("", Value{true});
    ASSERT_NE(table.tryGet(""), nullptr);
    EXPECT_TRUE(table.tryGet("")->as<bool>());
}

TEST(Table, MutableTryGetAllowsModification) {
    Table table;
    table.set("counter", Value{std::int64_t{1}});

    Value* value = table.tryGet("counter");
    ASSERT_NE(value, nullptr);
    value->as<std::int64_t>() = 2;

    EXPECT_EQ(table.tryGet("counter")->as<std::int64_t>(), 2);
}

TEST(Table, ConstTryGetOverloadIsSelectedOnConstTable) {
    Table table;
    table.set("key", Value{true});
    const Table& constTable = table;

    static_assert(std::is_same_v<decltype(constTable.tryGet("key")), const Value*>);
    static_assert(std::is_same_v<decltype(table.tryGet("key")), Value*>);
    EXPECT_NE(constTable.tryGet("key"), nullptr);
}

TEST(Table, IterationVisitsEveryEntry) {
    Table table;
    table.set("a", Value{std::int64_t{1}});
    table.set("b", Value{std::int64_t{2}});
    table.set("c", Value{std::int64_t{3}});

    std::set<std::string> keys;
    std::int64_t sum = 0;
    for (const auto& [key, value] : table) {
        keys.insert(key);
        sum += value.as<std::int64_t>();
    }

    EXPECT_EQ(keys, (std::set<std::string>{"a", "b", "c"}));
    EXPECT_EQ(sum, 6);
    EXPECT_EQ(static_cast<std::size_t>(std::distance(table.begin(), table.end())), table.size());
}

TEST(Table, ConstIterationVisitsEveryEntry) {
    Table table;
    table.set("a", Value{true});
    table.set("b", Value{false});
    const Table& constTable = table;

    std::size_t count = 0;
    for (auto it = constTable.cbegin(); it != constTable.cend(); ++it) {
        ++count;
    }
    for (auto it = constTable.begin(); it != constTable.end(); ++it) {
        ++count;
    }

    EXPECT_EQ(count, 4U);
}

TEST(Table, MutableIterationAllowsValueModification) {
    Table table;
    table.set("a", Value{std::int64_t{1}});
    table.set("b", Value{std::int64_t{2}});

    for (auto& [key, value] : table) {
        value.as<std::int64_t>() *= 10;
    }

    EXPECT_EQ(table.tryGet("a")->as<std::int64_t>(), 10);
    EXPECT_EQ(table.tryGet("b")->as<std::int64_t>(), 20);
}

TEST(Table, CopyConstructorDuplicatesEntries) {
    Table original;
    original.set("key", Value{std::string{"value"}});

    Table copy{original};
    copy.set("key", Value{std::string{"changed"}});
    copy.set("extra", Value{true});

    EXPECT_EQ(original.size(), 1U);
    EXPECT_EQ(original.tryGet("key")->as<std::string>(), "value");
    EXPECT_EQ(copy.size(), 2U);
    EXPECT_EQ(copy.tryGet("key")->as<std::string>(), "changed");
}

TEST(Table, CopyAssignmentReplacesContent) {
    Table source;
    source.set("new", Value{1.0});
    Table target;
    target.set("old", Value{2.0});

    target = source;

    EXPECT_EQ(target.size(), 1U);
    EXPECT_EQ(target.tryGet("old"), nullptr);
    EXPECT_NE(target.tryGet("new"), nullptr);
    EXPECT_EQ(source.size(), 1U);
}

TEST(Table, MoveConstructorTransfersEntries) {
    Table source;
    source.set("key", Value{std::string{"value"}});

    const Table moved{std::move(source)};

    EXPECT_EQ(moved.size(), 1U);
    ASSERT_NE(moved.tryGet("key"), nullptr);
    EXPECT_EQ(moved.tryGet("key")->as<std::string>(), "value");
}

TEST(Table, MoveAssignmentTransfersEntries) {
    Table source;
    source.set("new", Value{1.0});
    Table target;
    target.set("old", Value{2.0});

    target = std::move(source);

    EXPECT_EQ(target.size(), 1U);
    EXPECT_EQ(target.tryGet("old"), nullptr);
    EXPECT_NE(target.tryGet("new"), nullptr);
}

TEST(Table, IsCopyableAndMovable) {
    static_assert(std::is_copy_constructible_v<Table>);
    static_assert(std::is_copy_assignable_v<Table>);
    static_assert(std::is_nothrow_move_constructible_v<Table>);
    static_assert(std::is_nothrow_move_assignable_v<Table>);
}
