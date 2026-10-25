/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TableTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

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
using ErrorKind = rtype::luau::ErrorKind;
using Function = rtype::luau::Function;
using Table = rtype::luau::Table;
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

/// @brief Runs a Luau chunk that receives @p args as `...`, and returns what it returns.
ValuePack run(const rtype::luau::Runtime& runtime, const std::string& source, const ValuePack& args = {}) {
    const auto script = runtime.load("table", source);
    EXPECT_TRUE(script);
    script->closure().push(runtime.state());
    const Function function = Function::fromRef(rtype::luau::Ref::pop(runtime.state()), "table");
    auto results = function.call(args);
    EXPECT_TRUE(results) << results.error().message();
    return std::move(*results);
}

/// @brief Runs a Luau chunk and returns the table it returns.
Table runForTable(const rtype::luau::Runtime& runtime, const std::string& source) {
    const ValuePack results = run(runtime, source);
    EXPECT_EQ(results.size(), 1U);
    const Table* table = results.values()[0].tryAs<Table>();
    EXPECT_NE(table, nullptr);
    return *table;
}

/// @brief Reads a number from a table, failing the test if it is not there.
double number(const Table& table, const std::string& key) {
    const auto value = table.get(key);
    EXPECT_TRUE(value);
    EXPECT_TRUE(value->is<double>());
    return value->as<double>();
}
}  // namespace

TEST(Table, IsEmptyWhenCreated) {
    const auto runtime = makeRuntime();

    const Table table = Table::create(runtime.state()).value();

    EXPECT_EQ(table.size().value(), 0U);
    EXPECT_EQ(table.count().value(), 0U);
}

TEST(Table, CreateLeavesTheStackUntouched) {
    const auto runtime = makeRuntime();
    ValuePack probe{};
    probe.add({1.0});
    ASSERT_EQ(probe.push(runtime.state()).value(), 1U);

    const Table table = Table::create(runtime.state(), 4, 4).value();

    EXPECT_TRUE(Value::pop(runtime.state()));
    EXPECT_EQ(table.count().value(), 0U);
}

TEST(Table, CreateThrowsForNullState) {
    EXPECT_THROW(static_cast<void>(Table::create(nullptr)), rtype::luau::exceptions::InvalidState);
}

TEST(Table, SetStoresStringKey) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();

    ASSERT_TRUE(table.set("answer", Value{42.0}));

    const auto value = table.get("answer");
    ASSERT_TRUE(value);
    EXPECT_DOUBLE_EQ(value->as<double>(), 42.0);
    EXPECT_EQ(table.count().value(), 1U);
}

TEST(Table, SetOverwritesExistingKey) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    ASSERT_TRUE(table.set("key", Value{1.0}));

    ASSERT_TRUE(table.set("key", Value{std::string{"replaced"}}));

    EXPECT_EQ(table.count().value(), 1U);
    EXPECT_EQ(table.get("key")->as<std::string>(), "replaced");
}

TEST(Table, GetReturnsNilForMissingKey) {
    const auto runtime = makeRuntime();
    const Table table = Table::create(runtime.state()).value();

    const auto value = table.get("absent");

    ASSERT_TRUE(value);
    EXPECT_TRUE(value->is<Value::Nil>());
}

TEST(Table, KeysAreCaseSensitiveAndMayBeEmpty) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    ASSERT_TRUE(table.set("Key", Value{1.0}));
    ASSERT_TRUE(table.set("", Value{2.0}));

    EXPECT_TRUE(table.get("key")->is<Value::Nil>());
    EXPECT_DOUBLE_EQ(number(table, "Key"), 1.0);
    EXPECT_DOUBLE_EQ(number(table, ""), 2.0);
}

TEST(Table, SettingNilRemovesTheEntry) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    ASSERT_TRUE(table.set("key", Value{true}));

    ASSERT_TRUE(table.set("key", Value{Value::Nil{}}));

    EXPECT_EQ(table.count().value(), 0U);
    EXPECT_FALSE(*table.has("key"));
}

TEST(Table, HasTellsPresentFromAbsentKeys) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    ASSERT_TRUE(table.set("present", Value{false}));

    EXPECT_TRUE(*table.has("present"));
    EXPECT_TRUE(*table.has(Value{std::string{"present"}}));
    EXPECT_FALSE(*table.has("absent"));
}

TEST(Table, ArrayIndicesAreOneBased) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();

    for (std::size_t i = 1; i <= 3; ++i) {
        ASSERT_TRUE(table.setAt(i, Value{static_cast<double>(i) * 10.0}));
    }

    EXPECT_EQ(table.size().value(), 3U);
    EXPECT_EQ(table.count().value(), 3U);
    EXPECT_DOUBLE_EQ(table.getAt(1)->as<double>(), 10.0);
    EXPECT_DOUBLE_EQ(table.getAt(3)->as<double>(), 30.0);
    EXPECT_TRUE(table.getAt(0)->is<Value::Nil>());
    EXPECT_TRUE(table.getAt(4)->is<Value::Nil>());
}

TEST(Table, AcceptsKeysOfAnySupportedType) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    const Value vectorKey{Value::Vector{1.0F, 2.0F, 3.0F}};
    const Value tableKey{Table::create(runtime.state()).value()};

    ASSERT_TRUE(table.set(Value{true}, Value{1.0}));
    ASSERT_TRUE(table.set(Value{2.5}, Value{2.0}));
    ASSERT_TRUE(table.set(vectorKey, Value{3.0}));
    ASSERT_TRUE(table.set(tableKey, Value{4.0}));

    EXPECT_DOUBLE_EQ(table.get(Value{true})->as<double>(), 1.0);
    EXPECT_DOUBLE_EQ(table.get(Value{2.5})->as<double>(), 2.0);
    EXPECT_DOUBLE_EQ(table.get(vectorKey)->as<double>(), 3.0);
    EXPECT_DOUBLE_EQ(table.get(tableKey)->as<double>(), 4.0);
    EXPECT_EQ(table.count().value(), 4U);
}

TEST(Table, SetRejectsNilKey) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();

    const auto result = table.set(Value{Value::Nil{}}, Value{1.0});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), ErrorKind::kTypeMismatch);
    EXPECT_EQ(table.count().value(), 0U);
}

TEST(Table, SetRejectsNaNKey) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();

    const auto result = table.set(Value{std::numeric_limits<double>::quiet_NaN()}, Value{1.0});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), ErrorKind::kTypeMismatch);
}

TEST(Table, SetFailsOnReadOnlyTable) {
    const auto runtime = makeRuntime();
    Table table = runForTable(runtime, "return table.freeze({a = 1})");

    const auto result = table.set("a", Value{2.0});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), ErrorKind::kRuntime);
    EXPECT_DOUBLE_EQ(number(table, "a"), 1.0);
}

TEST(Table, SetFailsForTableOfAnotherState) {
    const auto runtime = makeRuntime();
    const auto foreign = makeRuntime();
    Table table = Table::create(runtime.state()).value();

    const auto result = table.set("other", Value{Table::create(foreign.state()).value()});

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().kind(), ErrorKind::kRuntime);
    EXPECT_EQ(table.count().value(), 0U);
}

TEST(Table, FailedOperationsLeaveTheStackAsTheyFoundIt) {
    const auto runtime = makeRuntime();
    const auto foreign = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    const Value foreignTable{Table::create(foreign.state()).value()};
    constexpr int kCalls = 10000;

    // The stack of Luau is capped at 8000 slots: leaking one slot per call would overflow it.
    for (int i = 0; i < kCalls; ++i) {
        ASSERT_FALSE(table.set(Value{Value::Nil{}}, Value{1.0}));
        // These fail once the table is already on the stack, so they exercise the cleanup after stack access.
        ASSERT_FALSE(table.set("other", foreignTable));
        ASSERT_FALSE(table.get(foreignTable));
        ASSERT_TRUE(table.set("key", Value{1.0}));
        ASSERT_TRUE(table.get("key"));
        ASSERT_TRUE(table.entries());
    }
}

TEST(Table, CopyRefersToTheSameTable) {
    const auto runtime = makeRuntime();
    Table original = Table::create(runtime.state()).value();
    Table copy{original};

    ASSERT_TRUE(copy.set("shared", Value{1.0}));
    ASSERT_TRUE(original.set("other", Value{2.0}));

    EXPECT_DOUBLE_EQ(number(original, "shared"), 1.0);
    EXPECT_DOUBLE_EQ(number(copy, "other"), 2.0);
    EXPECT_TRUE(original.refersToSame(copy).value());
}

TEST(Table, CopyAssignmentRefersToTheOtherTable) {
    const auto runtime = makeRuntime();
    const Table source = Table::create(runtime.state()).value();
    Table target = Table::create(runtime.state()).value();

    target = source;

    EXPECT_TRUE(target.refersToSame(source).value());
}

TEST(Table, DistinctTablesDoNotReferToTheSame) {
    const auto runtime = makeRuntime();
    const Table first = Table::create(runtime.state()).value();
    const Table second = Table::create(runtime.state()).value();
    const auto foreign = makeRuntime();
    const Table other = Table::create(foreign.state()).value();

    EXPECT_FALSE(first.refersToSame(second).value());
    EXPECT_FALSE(first.refersToSame(other).value());
}

TEST(Table, MovedFromHandleThrowsInvalidRef) {
    const auto runtime = makeRuntime();
    Table source = Table::create(runtime.state()).value();

    const Table moved{std::move(source)};

    EXPECT_EQ(moved.count().value(), 0U);
    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
    EXPECT_THROW(static_cast<void>(source.count().value()), rtype::luau::exceptions::InvalidRef);
}

TEST(Table, IsCheapToCopyAndMove) {
    static_assert(std::is_nothrow_copy_constructible_v<Table>);
    static_assert(std::is_nothrow_copy_assignable_v<Table>);
    static_assert(std::is_nothrow_move_constructible_v<Table>);
    static_assert(std::is_nothrow_move_assignable_v<Table>);
    static_assert(!std::is_default_constructible_v<Table>);
}

TEST(Table, ScriptSeesChangesMadeFromCpp) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    ASSERT_TRUE(table.set("hp", Value{10.0}));
    ValuePack args{};
    args.add({table});

    const ValuePack results = run(runtime, "local t = ...; return t.hp * 2", args);

    ASSERT_EQ(results.size(), 1U);
    EXPECT_DOUBLE_EQ(results.values()[0].as<double>(), 20.0);
}

TEST(Table, CppSeesChangesMadeByScript) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    ValuePack args{};
    args.add({table});

    static_cast<void>(run(runtime, "local t = ...; t.hp = 99; t[1] = 'first'", args));

    EXPECT_DOUBLE_EQ(number(table, "hp"), 99.0);
    EXPECT_EQ(table.getAt(1)->as<std::string>(), "first");
}

TEST(Table, ReadsTableReturnedByScript) {
    const auto runtime = makeRuntime();

    const Table table = runForTable(runtime, "return {name = 'r-type', lives = 3, {1, 2}}");

    EXPECT_EQ(table.get("name")->as<std::string>(), "r-type");
    EXPECT_DOUBLE_EQ(number(table, "lives"), 3.0);
    EXPECT_EQ(table.size().value(), 1U);
    EXPECT_EQ(table.count().value(), 3U);
}

TEST(Table, NestedTablesAreLiveHandles) {
    const auto runtime = makeRuntime();
    const Table outer = runForTable(runtime, "return {inner = {x = 1}}");

    const auto inner = outer.get("inner");
    ASSERT_TRUE(inner);
    ASSERT_TRUE(inner->is<Table>());
    Table handle = inner->as<Table>();
    ASSERT_TRUE(handle.set("x", Value{5.0}));

    EXPECT_DOUBLE_EQ(number(outer.get("inner")->as<Table>(), "x"), 5.0);
    EXPECT_TRUE(handle.refersToSame(outer.get("inner")->as<Table>()).value());
}

TEST(Table, PushThenFromStackGivesTheSameTable) {
    const auto runtime = makeRuntime();
    const Table table = Table::create(runtime.state()).value();

    ASSERT_TRUE(table.push(runtime.state()));
    const auto read = Table::fromStack(runtime.state());

    ASSERT_TRUE(read);
    EXPECT_TRUE(read->refersToSame(table).value());
    EXPECT_TRUE(Value::pop(runtime.state()));
}

TEST(Table, FromStackFailsForOtherTypes) {
    const auto runtime = makeRuntime();
    ValuePack pack{};
    pack.add({1.0});
    ASSERT_EQ(pack.push(runtime.state()).value(), 1U);

    const auto read = Table::fromStack(runtime.state());

    ASSERT_FALSE(read);
    EXPECT_EQ(read.error().kind(), ErrorKind::kTypeMismatch);
}

TEST(Table, PushFailsForTableOfAnotherState) {
    const auto runtime = makeRuntime();
    const auto foreign = makeRuntime();
    const Table table = Table::create(foreign.state()).value();

    const auto pushed = table.push(runtime.state());

    ASSERT_FALSE(pushed);
    EXPECT_EQ(pushed.error().kind(), rtype::luau::ErrorKind::kRuntime);
}

TEST(Table, AccessIgnoresMetatables) {
    const auto runtime = makeRuntime();
    Table table = runForTable(runtime, R"(
        return setmetatable({}, {
            __index = function() return 1 end,
            __newindex = function() error("should not run") end,
        })
    )");

    EXPECT_TRUE(table.get("missing")->is<Value::Nil>());
    EXPECT_TRUE(table.set("key", Value{2.0}));
    EXPECT_DOUBLE_EQ(number(table, "key"), 2.0);
}

TEST(Table, EntriesCopyEveryPair) {
    const auto runtime = makeRuntime();
    const Table table = runForTable(runtime, "return {a = 1, b = 2, 'x'}");

    const auto entries = table.entries();

    ASSERT_TRUE(entries);
    ASSERT_EQ(entries->size(), 3U);
    double sum = 0.0;
    for (const auto& [key, value] : *entries) {
        if (key.is<std::string>()) {
            sum += value.as<double>();
        } else {
            EXPECT_DOUBLE_EQ(key.as<double>(), 1.0);
            EXPECT_EQ(value.as<std::string>(), "x");
        }
    }
    EXPECT_DOUBLE_EQ(sum, 3.0);
}

TEST(Table, EntriesFailForUnreadableValue) {
    const auto runtime = makeRuntime();
    const Table table = runForTable(runtime, "return {f = function() end}");

    const auto entries = table.entries();

    ASSERT_FALSE(entries);
    EXPECT_EQ(entries.error().kind(), ErrorKind::kTypeMismatch);
    EXPECT_FALSE(table.get("f"));
}

TEST(Table, ToMapCopiesStringKeyedEntries) {
    const auto runtime = makeRuntime();
    const Table table = runForTable(runtime, "return {a = 1, b = 'two', c = {}}");

    const auto map = table.toMap();

    ASSERT_TRUE(map);
    EXPECT_EQ(map->size(), 3U);
    EXPECT_DOUBLE_EQ(map->find("a")->second.as<double>(), 1.0);
    EXPECT_EQ(map->find(std::string_view{"b"})->second.as<std::string>(), "two");
    EXPECT_TRUE(map->find("c")->second.is<Table>());
}

TEST(Table, ToMapIsASnapshot) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    ASSERT_TRUE(table.set("key", Value{1.0}));

    const auto map = table.toMap();
    ASSERT_TRUE(table.set("key", Value{2.0}));
    ASSERT_TRUE(table.set("added", Value{3.0}));

    ASSERT_TRUE(map);
    EXPECT_EQ(map->size(), 1U);
    EXPECT_DOUBLE_EQ(map->at("key").as<double>(), 1.0);
}

TEST(Table, ToMapFailsForNonStringKey) {
    const auto runtime = makeRuntime();
    Table table = Table::create(runtime.state()).value();
    ASSERT_TRUE(table.set("name", Value{1.0}));
    ASSERT_TRUE(table.setAt(1, Value{2.0}));

    const auto map = table.toMap();

    ASSERT_FALSE(map);
    EXPECT_EQ(map.error().kind(), ErrorKind::kTypeMismatch);
}

TEST(Table, ToMapOfEmptyTableIsEmpty) {
    const auto runtime = makeRuntime();
    const Table table = Table::create(runtime.state()).value();

    const auto map = table.toMap();

    ASSERT_TRUE(map);
    EXPECT_TRUE(map->empty());
}
