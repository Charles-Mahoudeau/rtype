/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** UsertypeTests
*/

#include <gtest/gtest.h>
#include <lua.h>

#include <format>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

#include "rtype/luau/ErrorKind.hpp"
#include "rtype/luau/Exceptions.hpp"
#include "rtype/luau/Function.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Usertype.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/luau/ValuePack.hpp"

namespace {
using Userdata = rtype::luau::Userdata;
using Value = rtype::luau::Value;
using ValuePack = rtype::luau::ValuePack;

/// @brief A C++ type exposed to the scripts. It keeps a `tracker` alive, to see when it is destroyed.
struct Counter {
    double value{0};
    std::shared_ptr<int> tracker;
};

/// @brief Another C++ type, to give a method the wrong kind of userdata.
struct Other {
    double value{0};
};

/// @brief A C++ type without `__eq`.
struct Plain {
    double value{0};
};

/// @brief A C++ type whose members are known at runtime.
struct Bag {
    std::map<std::string, double, std::less<>> fields;
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

/// @brief Registers `Counter` with a constructor, a read-write `value`, a read-only `double`, methods, `__eq` and
///        `__tostring`.
rtype::luau::Usertype<Counter> registerCounter(rtype::luau::Runtime& runtime,
                                               const std::shared_ptr<int>& tracker = {}) {
    return runtime.usertype<Counter>("Counter")
        .constructor([tracker](const double start) { return Counter{.value = start, .tracker = tracker}; })
        .getter("value", [](const Counter& counter) { return counter.value; })
        .setter("value", [](Counter& counter, const double value) { counter.value = value; })
        .getter("double", [](const Counter& counter) { return counter.value * 2; })
        .method("add",
                [](Counter& counter, const double amount) {
                    counter.value += amount;
                    return counter.value;
                })
        .method("addOptional",
                [](Counter& counter, const std::optional<double> amount) {
                    counter.value += amount.value_or(1);
                    return counter.value;
                })
        .method("reset", [](Counter& counter) { counter.value = 0; })
        .eq([](const Counter& lhs, const Counter& rhs) { return lhs.value == rhs.value; })
        .tostring([](const Counter& counter) { return std::format("Counter({})", counter.value); });
}

/// @brief Runs @p source, failing the test if it raises an error.
void run(const rtype::luau::Runtime& runtime, const std::string& source) {
    const auto script = runtime.load("usertype-test", source);
    ASSERT_TRUE(script);
    const auto result = runtime.run(*script);
    ASSERT_TRUE(result) << result.error().message();
}

/// @brief Runs @p source, which must raise an error, and returns its message.
std::string runForError(const rtype::luau::Runtime& runtime, const std::string& source) {
    const auto script = runtime.load("usertype-test", source);
    EXPECT_TRUE(script);
    const auto result = runtime.run(*script);
    EXPECT_FALSE(result);
    if (result) {
        return {};
    }
    EXPECT_EQ(result.error().kind(), rtype::luau::ErrorKind::kRuntime);
    return std::string{result.error().message()};
}

/// @brief Reads a global of the runtime as a number.
double globalNumber(const rtype::luau::Runtime& runtime, const std::string_view name) {
    const auto value = runtime.globals().get(name);
    EXPECT_TRUE(value);
    const auto* number = value->tryAs<double>();
    EXPECT_NE(number, nullptr);
    return number != nullptr ? *number : 0;
}
}  // namespace

TEST(Usertype, ReadsAndWritesAMemberFromAScript) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    run(runtime, R"(
        local counter = Counter.new(3)
        before = counter.value
        counter.value = 10
        after = counter.value
        computed = counter.double
    )");

    EXPECT_EQ(globalNumber(runtime, "before"), 3);
    EXPECT_EQ(globalNumber(runtime, "after"), 10);
    EXPECT_EQ(globalNumber(runtime, "computed"), 20);
}

TEST(Usertype, CallsAMethod) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    run(runtime, R"(
        local counter = Counter.new(1)
        result = counter:add(4)
        counter:reset()
        afterReset = counter.value
    )");

    EXPECT_EQ(globalNumber(runtime, "result"), 5);
    EXPECT_EQ(globalNumber(runtime, "afterReset"), 0);
}

TEST(Usertype, AcceptsAMissingOptionalArgument) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    run(runtime, R"(
        local counter = Counter.new(0)
        counter:addOptional()
        counter:addOptional(5)
        result = counter.value
    )");

    EXPECT_EQ(globalNumber(runtime, "result"), 6);
}

TEST(Usertype, GivesTheNameAsTypeOfItsUserdata) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    run(runtime, R"(
        local counter = Counter.new(1)
        kind = typeof(counter)
        text = tostring(counter)
        locked = getmetatable(counter)
    )");

    EXPECT_EQ(runtime.globals().get("kind")->as<std::string>(), "Counter");
    EXPECT_EQ(runtime.globals().get("text")->as<std::string>(), "Counter(1)");
    EXPECT_EQ(runtime.globals().get("locked")->as<std::string>(), "locked");
}

TEST(Usertype, RaisesAnErrorWhenAMethodGetsNilAsObject) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    const auto message = runForError(runtime, R"(
        local counter = Counter.new(1)
        local add = counter.add
        add(nil, 1)
    )");

    EXPECT_NE(message.find("Counter:add"), std::string::npos) << message;
    EXPECT_NE(message.find("self is not a Counter (got nil)"), std::string::npos) << message;
}

TEST(Usertype, RaisesAnErrorWhenAMethodGetsAnotherUserdataAsObject) {
    auto runtime = makeRuntime();
    registerCounter(runtime);
    runtime.usertype<Other>("Other").constructor([](const double value) { return Other{.value = value}; });

    const auto message = runForError(runtime, R"(
        local counter = Counter.new(1)
        local other = Other.new(2)
        counter.add(other, 1)
    )");

    EXPECT_NE(message.find("self is not a Counter (got userdata)"), std::string::npos) << message;
}

TEST(Usertype, RaisesAnErrorWhenAnArgumentHasTheWrongType) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    const auto message = runForError(runtime, R"(
        Counter.new(1):add("many")
    )");

    EXPECT_NE(message.find("Counter:add"), std::string::npos) << message;
    EXPECT_NE(message.find("argument 1: expected number, got string"), std::string::npos) << message;
}

TEST(Usertype, RaisesAnErrorWhenTheArgumentCountIsWrong) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    const auto message = runForError(runtime, R"(
        Counter.new(1):add(1, 2)
    )");

    EXPECT_NE(message.find("expected 1 arguments, got 2"), std::string::npos) << message;
}

TEST(Usertype, RaisesAnErrorWhenTheConstructorGetsTheWrongType) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    const auto message = runForError(runtime, R"(
        Counter.new("one")
    )");

    EXPECT_NE(message.find("Counter.new"), std::string::npos) << message;
    EXPECT_NE(message.find("expected number, got string"), std::string::npos) << message;
}

TEST(Usertype, RaisesAnErrorForAnUnknownMember) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    EXPECT_NE(runForError(runtime, "local x = Counter.new(1).nope").find("unknown member 'nope'"), std::string::npos);
    EXPECT_NE(runForError(runtime, "Counter.new(1).nope = 1").find("unknown member 'nope'"), std::string::npos);
}

TEST(Usertype, RaisesAnErrorWhenWritingAReadOnlyMember) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    const auto message = runForError(runtime, "Counter.new(1).double = 4");

    EXPECT_NE(message.find("member 'double' is read-only"), std::string::npos) << message;
}

TEST(Usertype, RaisesAnErrorWhenAMemberIsNotAString) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    const auto message = runForError(runtime, "local x = Counter.new(1)[1]");

    EXPECT_NE(message.find("members are named by strings, got number"), std::string::npos) << message;
}

TEST(Usertype, ComparesWithTheEqualityOfTheType) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    run(runtime, R"(
        same = Counter.new(1) == Counter.new(1)
        different = Counter.new(1) == Counter.new(2)
        withNumber = Counter.new(1) == 1
    )");

    EXPECT_TRUE(runtime.globals().get("same")->as<bool>());
    EXPECT_FALSE(runtime.globals().get("different")->as<bool>());
    EXPECT_FALSE(runtime.globals().get("withNumber")->as<bool>());
}

TEST(Usertype, ComparesByIdentityWithoutEquality) {
    auto runtime = makeRuntime();
    runtime.usertype<Plain>("Plain").constructor([](const double value) { return Plain{.value = value}; });

    run(runtime, R"(
        local first = Plain.new(1)
        local second = Plain.new(1)
        sameObject = first == first
        sameValue = first == second
    )");

    EXPECT_TRUE(runtime.globals().get("sameObject")->as<bool>());
    EXPECT_FALSE(runtime.globals().get("sameValue")->as<bool>());
}

TEST(Usertype, DestroysItsObjectsWhenTheRuntimeIsClosed) {
    const auto tracker = std::make_shared<int>();
    {
        auto runtime = makeRuntime();
        registerCounter(runtime, tracker);
        run(runtime, R"(
            kept = Counter.new(1)
            other = Counter.new(2)
        )");
        EXPECT_EQ(tracker.use_count(), 4);  // the test, the constructor handler and the two objects
    }

    EXPECT_EQ(tracker.use_count(), 1);
}

TEST(Usertype, DestroysItsObjectsWhenTheyAreCollected) {
    const auto tracker = std::make_shared<int>();
    auto runtime = makeRuntime();
    registerCounter(runtime, tracker);

    run(runtime, R"(
        Counter.new(1)
        Counter.new(2)
    )");
    // `collectgarbage` does not work in scripts of this runtime yet (it fails with or without usertypes).
    lua_gc(runtime.state(), LUA_GCCOLLECT, 0);

    EXPECT_EQ(tracker.use_count(), 2);  // the test and the constructor handler
}

TEST(Usertype, ReadsMembersKnownAtRuntimeThroughTheFallbacks) {
    auto runtime = makeRuntime();
    runtime.usertype<Bag>("Bag")
        .constructor([] { return Bag{}; })
        .index([](Bag& bag, const std::string_view key) -> Value {
            const auto field = bag.fields.find(key);
            if (field == bag.fields.end()) {
                throw rtype::luau::exceptions::RuntimeError{std::format("no field '{}'", key)};
            }
            return Value{field->second};
        })
        .newIndex([](Bag& bag, const std::string_view key, const Value& value) {
            const auto* number = value.tryAs<double>();
            if (number == nullptr) {
                throw rtype::luau::exceptions::RuntimeError{"fields are numbers"};
            }
            bag.fields.insert_or_assign(std::string{key}, *number);
        });

    run(runtime, R"(
        local bag = Bag.new()
        bag.strength = 7
        result = bag.strength
    )");
    const auto missing = runForError(runtime, "local x = Bag.new().strength");
    const auto wrongType = runForError(runtime, "Bag.new().strength = 'strong'");

    EXPECT_EQ(globalNumber(runtime, "result"), 7);
    EXPECT_NE(missing.find("Bag.strength: no field 'strength'"), std::string::npos) << missing;
    EXPECT_NE(wrongType.find("fields are numbers"), std::string::npos) << wrongType;
}

TEST(Usertype, PrefersGettersAndMethodsToTheFallback) {
    auto runtime = makeRuntime();
    runtime.usertype<Bag>("Bag")
        .constructor([] { return Bag{}; })
        .getter("size", [](const Bag& bag) { return static_cast<double>(bag.fields.size()); })
        .index([](Bag&, const std::string_view) -> Value { return Value{-1.0}; });

    run(runtime, R"(
        local bag = Bag.new()
        known = bag.size
        other = bag.anything
    )");

    EXPECT_EQ(globalNumber(runtime, "known"), 0);
    EXPECT_EQ(globalNumber(runtime, "other"), -1);
}

TEST(Usertype, HandsAnObjectCreatedByCppToScripts) {
    auto runtime = makeRuntime();
    const auto counter = registerCounter(runtime);

    auto userdata = counter.create(Counter{.value = 41});
    ASSERT_TRUE(userdata);
    ASSERT_TRUE(runtime.globals().set("given", Value{*userdata}));
    run(runtime, R"(
        given:add(1)
        result = given.value
    )");

    EXPECT_EQ(globalNumber(runtime, "result"), 42);
}

TEST(Usertype, LetsCppReadTheObjectOfAUserdataItReceived) {
    auto runtime = makeRuntime();
    registerCounter(runtime);
    Counter* seen = nullptr;
    auto inspect = rtype::luau::Function::create(runtime.state(), "inspect");
    inspect.bind([&seen](const Userdata& userdata) {
        seen = userdata.tryAs<Counter>();
        if (seen != nullptr) {
            seen->value += 100;
        }
        return ValuePack{};
    });
    ASSERT_TRUE(runtime.globals().set("inspect", Value{inspect.ref()}));

    run(runtime, R"(
        local counter = Counter.new(1)
        inspect(counter)
        result = counter.value
    )");

    ASSERT_NE(seen, nullptr);
    EXPECT_EQ(globalNumber(runtime, "result"), 101);
}

TEST(Usertype, RefusesToReadTheObjectAsAnotherType) {
    auto runtime = makeRuntime();
    const auto counter = registerCounter(runtime);
    std::ignore = runtime.usertype<Other>("Other");

    const auto userdata = counter.create(Counter{.value = 1});

    ASSERT_TRUE(userdata);
    EXPECT_NE(userdata->tryAs<Counter>(), nullptr);
    EXPECT_EQ(userdata->tryAs<Other>(), nullptr);
    EXPECT_EQ(userdata->tryAs<Bag>(), nullptr);  // never registered
}

TEST(Usertype, GivesEachTypeItsOwnTag) {
    auto runtime = makeRuntime();
    const auto counter = registerCounter(runtime);
    const auto other = runtime.usertype<Other>("Other");

    const auto first = counter.create(Counter{});
    const auto second = other.create(Other{});

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    EXPECT_NE(first->tag(), second->tag());
}

TEST(Usertype, RefusesToPushAUserdataInAnotherRuntime) {
    auto runtime = makeRuntime();
    auto foreign = makeRuntime();
    const auto userdata = registerCounter(runtime).create(Counter{});
    ASSERT_TRUE(userdata);

    const auto pushed = userdata->push(foreign.state());

    ASSERT_FALSE(pushed);
    EXPECT_EQ(pushed.error().kind(), rtype::luau::ErrorKind::kRuntime);
}

TEST(Usertype, IsRegisteredOncePerRuntime) {
    auto runtime = makeRuntime();
    registerCounter(runtime);

    EXPECT_THROW(std::ignore = runtime.usertype<Counter>("Again"), rtype::luau::exceptions::InvalidOperation);
    EXPECT_EQ(std::as_const(runtime).usertype<Counter>().name(), "Counter");
    EXPECT_THROW(std::ignore = std::as_const(runtime).usertype<Other>(), rtype::luau::exceptions::InvalidOperation);
}

TEST(Usertype, KeepsATypeSeparateInEachRuntime) {
    auto first = makeRuntime();
    auto second = makeRuntime();

    registerCounter(first);

    EXPECT_NO_THROW(std::ignore = second.usertype<Counter>("Counter"));
}

TEST(Usertype, SurvivesMovingTheRuntime) {
    auto runtime = makeRuntime();
    registerCounter(runtime);
    run(runtime, "kept = Counter.new(5)");

    auto moved = std::move(runtime);
    auto other = makeRuntime();
    registerCounter(other);
    other = std::move(moved);
    run(other, "result = kept:add(1)");

    EXPECT_EQ(globalNumber(other, "result"), 6);
    EXPECT_EQ(std::as_const(other).usertype<Counter>().name(), "Counter");
}

TEST(Usertype, RejectsAMethodNameWithAnEmbeddedNul) {
    auto runtime = makeRuntime();
    auto type = runtime.usertype<Counter>("Counter");

    EXPECT_THROW(std::ignore = type.method(std::string{"a\0b", 3}, [](Counter&) {}),
                 rtype::luau::exceptions::InvalidOperation);
}

TEST(Usertype, RejectsAConstructorWhoseNameIsAlreadyAGlobal) {
    auto runtime = makeRuntime();
    auto type = runtime.usertype<Counter>("math");

    EXPECT_THROW(std::ignore = type.constructor([] { return Counter{}; }), rtype::luau::exceptions::InvalidOperation);
    run(runtime, "result = math.max(1, 2)");
    EXPECT_EQ(globalNumber(runtime, "result"), 2);
}

TEST(Usertype, AllowsSettingTheConstructorOfATypeTwice) {
    auto runtime = makeRuntime();
    auto type = runtime.usertype<Counter>("Counter");

    type.constructor([] { return Counter{.value = 1}; });
    EXPECT_NO_THROW(std::ignore = type.constructor([] { return Counter{.value = 2}; }));
}
