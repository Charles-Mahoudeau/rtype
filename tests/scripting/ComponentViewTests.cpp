/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentViewTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/world/World.hpp"
#include "rtype/luau/Runtime.hpp"
#include "rtype/luau/RuntimeConfig.hpp"
#include "rtype/luau/Table.hpp"
#include "rtype/luau/Userdata.hpp"
#include "rtype/luau/Value.hpp"
#include "rtype/scripting/ErrorKind.hpp"
#include "rtype/scripting/component/ComponentDeclaration.hpp"
#include "rtype/scripting/component/ComponentRegistration.hpp"
#include "rtype/scripting/view/ComponentView.hpp"
#include "rtype/scripting/view/FieldAccess.hpp"
#include "rtype/scripting/view/ViewScope.hpp"
#include "rtype/scripting/view/ViewTypes.hpp"

namespace {
namespace sc = rtype::scripting::component;
namespace sv = rtype::scripting::view;
using rtype::ecs::FieldType;

/// @brief A registered component with its description and a value, the pieces a view is made of.
struct Fixture {
    rtype::ecs::World world;
    rtype::luau::Runtime runtime{makeRuntime()};
    rtype::ecs::ComponentInfo info;
    std::vector<std::byte> bytes;
    sv::ViewScope scope;

    explicit Fixture(const sc::ComponentDeclaration& declaration) {
        const auto layout = sc::registerComponent(world.getComponents(), declaration);
        EXPECT_TRUE(layout);
        info.type.name = declaration.name;
        info.type.size = layout->size;
        info.type.alignment = layout->alignment;
        info.type.fields = layout->fields;
        bytes = layout->defaults;
        EXPECT_TRUE(sv::registerViewTypes(runtime));
    }

    static rtype::luau::Runtime makeRuntime() {
        auto created = rtype::luau::Runtime::create({
            .libs = rtype::luau::RuntimeConfig::Libs::kStandard,
            .optimizationLevel = rtype::luau::RuntimeConfig::OptimizationLevel::kStandard,
        });
        EXPECT_TRUE(created);
        return std::move(*created);
    }

    /// @brief Makes the view of the component's bytes a global of the runtime.
    void expose(const std::string& name, const bool readOnly = false) {
        const auto userdata = runtime.usertype<sv::ComponentView>().create(
            {.info = &info, .bytes = bytes, .scope = &scope, .generation = scope.generation, .readOnly = readOnly});
        ASSERT_TRUE(userdata);
        ASSERT_TRUE(runtime.globals().set(name, rtype::luau::Value{*userdata}));
    }

    /// @brief Runs a script and returns the message of its error, empty when it succeeded.
    std::string run(const std::string& source) {
        const auto script = runtime.load("view-test", source);
        EXPECT_TRUE(script);
        const auto result = runtime.run(*script);
        return result ? std::string{} : std::string{result.error().message()};
    }

    double number(const std::string_view name) const {
        const auto value = runtime.globals().get(name);
        EXPECT_TRUE(value);
        const auto* found = value->tryAs<double>();
        EXPECT_NE(found, nullptr);
        return found != nullptr ? *found : 0;
    }

    template <typename T>
    T read(const std::string_view field) const {
        for (const auto& candidate : info.type.fields) {
            if (candidate.name == field) {
                T value{};
                std::memcpy(&value, bytes.data() + candidate.offset, sizeof(T));
                return value;
            }
        }
        return {};
    }
};

sc::ComponentDeclaration makeEverything() {
    return {.name = "test.Everything",
            .fields = {{.name = "flag", .type = FieldType::kBool},
                       {.name = "i8", .type = FieldType::kI8},
                       {.name = "i16", .type = FieldType::kI16},
                       {.name = "i32", .type = FieldType::kI32},
                       {.name = "i64", .type = FieldType::kI64},
                       {.name = "u8", .type = FieldType::kU8},
                       {.name = "u16", .type = FieldType::kU16},
                       {.name = "u32", .type = FieldType::kU32},
                       {.name = "u64", .type = FieldType::kU64},
                       {.name = "f32", .type = FieldType::kF32},
                       {.name = "f64", .type = FieldType::kF64},
                       {.name = "target", .type = FieldType::kEntity}}};
}

sc::ComponentDeclaration makeShield() {
    return {.name = "game.Shield",
            .fields = {{.name = "strength", .type = FieldType::kF32}, {.name = "regen", .type = FieldType::kF32}}};
}
}  // namespace

TEST(ComponentView, RoundTripsEverySupportedFieldType) {
    Fixture fixture{makeEverything()};
    fixture.expose("v");

    const auto entity = fixture.runtime.usertype<rtype::ecs::Entity>().create(rtype::ecs::Entity{7, 3});
    ASSERT_TRUE(entity);
    ASSERT_TRUE(fixture.runtime.globals().set("other", rtype::luau::Value{*entity}));
    EXPECT_EQ(fixture.run(R"(
        v.flag = true
        v.i8 = -128
        v.i16 = -32768
        v.i32 = -2147483648
        v.i64 = -9007199254740992
        v.u8 = 255
        v.u16 = 65535
        v.u32 = 4294967295
        v.u64 = 9007199254740992
        v.f32 = 1.5
        v.f64 = 0.1
        v.target = other

        i8 = v.i8
        i16 = v.i16
        i32 = v.i32
        i64 = v.i64
        u8 = v.u8
        u16 = v.u16
        u32 = v.u32
        u64 = v.u64
        f32 = v.f32
        f64 = v.f64
        sameTarget = v.target == other
        isTrue = v.flag
    )"),
              "");

    EXPECT_EQ(fixture.number("i8"), -128);
    EXPECT_EQ(fixture.number("i16"), -32768);
    EXPECT_EQ(fixture.number("i32"), -2147483648.0);
    EXPECT_EQ(fixture.number("i64"), -9007199254740992.0);
    EXPECT_EQ(fixture.number("u8"), 255);
    EXPECT_EQ(fixture.number("u16"), 65535);
    EXPECT_EQ(fixture.number("u32"), 4294967295.0);
    EXPECT_EQ(fixture.number("u64"), 9007199254740992.0);
    EXPECT_EQ(fixture.number("f32"), 1.5);
    EXPECT_EQ(fixture.number("f64"), 0.1);
    EXPECT_EQ(fixture.read<std::int8_t>("i8"), -128);
    EXPECT_EQ(fixture.read<std::uint64_t>("u64"), 9007199254740992ULL);
    EXPECT_EQ(fixture.read<rtype::ecs::Entity>("target"), (rtype::ecs::Entity{7, 3}));
    const auto same = fixture.runtime.globals().get("sameTarget");
    ASSERT_TRUE(same);
    EXPECT_TRUE(*same->tryAs<bool>());
}

TEST(ComponentView, ReadsTheBytesInPlace) {
    Fixture fixture{makeShield()};
    fixture.expose("shield");
    const float strength = 42.0F;
    for (const auto& field : fixture.info.type.fields) {
        if (field.name == "strength") {
            std::memcpy(fixture.bytes.data() + field.offset, &strength, sizeof(strength));
        }
    }

    ASSERT_EQ(fixture.run("value = shield.strength; shield.regen = 3"), "");

    EXPECT_EQ(fixture.number("value"), 42);
    EXPECT_EQ(fixture.read<float>("regen"), 3.0F);
}

TEST(ComponentView, RefusesAnIntegerOutOfRangeAndNamesTheField) {
    Fixture fixture{makeEverything()};
    fixture.expose("v");

    const auto error = fixture.run("v.i8 = 300");

    EXPECT_NE(error.find("i8"), std::string::npos) << error;
    EXPECT_NE(error.find("out of range"), std::string::npos) << error;
    EXPECT_EQ(fixture.read<std::int8_t>("i8"), 0);
}

TEST(ComponentView, RefusesAnIntegerThatIsNotWhole) {
    Fixture fixture{makeEverything()};
    fixture.expose("v");

    EXPECT_NE(fixture.run("v.u16 = 1.5").find("whole number"), std::string::npos);
}

TEST(ComponentView, RefusesA64BitIntegerBeyondWhatANumberHoldsExactly) {
    Fixture fixture{makeEverything()};
    fixture.expose("v");

    EXPECT_NE(fixture.run("v.i64 = 9007199254740994").find("out of range"), std::string::npos);
    EXPECT_NE(fixture.run("v.u64 = -1").find("out of range"), std::string::npos);
}

TEST(ComponentView, RefusesToReadA64BitIntegerBeyondWhatANumberHoldsExactly) {
    Fixture fixture{makeEverything()};
    fixture.expose("v");
    const std::int64_t big = 9007199254740993;
    const std::uint64_t bigUnsigned = 18446744073709551615ULL;
    for (const auto& field : fixture.info.type.fields) {
        if (field.name == "i64") {
            std::memcpy(fixture.bytes.data() + field.offset, &big, sizeof(big));
        } else if (field.name == "u64") {
            std::memcpy(fixture.bytes.data() + field.offset, &bigUnsigned, sizeof(bigUnsigned));
        }
    }

    const auto signedError = fixture.run("value = v.i64");
    EXPECT_NE(signedError.find("i64"), std::string::npos) << signedError;
    EXPECT_NE(signedError.find("out of range"), std::string::npos) << signedError;
    EXPECT_NE(fixture.run("value = v.u64").find("out of range"), std::string::npos);
}

TEST(ComponentView, RefusesNilAWrongTypeAndAnUnknownField) {
    Fixture fixture{makeEverything()};
    fixture.expose("v");

    EXPECT_NE(fixture.run("v.f32 = nil").find("expected a number"), std::string::npos);
    EXPECT_NE(fixture.run("v.f32 = 'text'").find("expected a number"), std::string::npos);
    EXPECT_NE(fixture.run("v.flag = 1").find("expected a boolean"), std::string::npos);
    EXPECT_NE(fixture.run("v.target = 3").find("expected an Entity"), std::string::npos);
    EXPECT_NE(fixture.run("v.missing = 1").find("no field 'missing'"), std::string::npos);
    EXPECT_NE(fixture.run("x = v.missing").find("no field 'missing'"), std::string::npos);
}

TEST(ComponentView, ReadsANullEntityFieldAsTheNullEntity) {
    Fixture fixture{makeEverything()};
    fixture.expose("v");

    EXPECT_EQ(fixture.run("text = tostring(v.target)"), "");

    const auto text = fixture.runtime.globals().get("text");
    ASSERT_TRUE(text);
    EXPECT_EQ(*text->tryAs<std::string>(), "Entity(0:0)");
}

TEST(ComponentView, RefusesAccessOnceTheScopeMovedOn) {
    Fixture fixture{makeShield()};
    fixture.expose("shield");
    ASSERT_EQ(fixture.run("value = shield.strength"), "");

    ++fixture.scope.generation;

    EXPECT_NE(fixture.run("value = shield.strength").find("no longer valid"), std::string::npos);
    EXPECT_NE(fixture.run("shield.strength = 1").find("no longer valid"), std::string::npos);
}

TEST(ComponentView, RefusesWritesThroughAReadOnlyView) {
    Fixture fixture{makeShield()};
    fixture.expose("shield", true);

    EXPECT_EQ(fixture.run("value = shield.strength"), "");
    const auto error = fixture.run("shield.strength = 1");

    EXPECT_NE(error.find("read-only"), std::string::npos) << error;
    EXPECT_EQ(fixture.read<float>("strength"), 0.0F);
}

TEST(Entity, ComparesByValueAndShowsItsIndexAndGeneration) {
    Fixture fixture{makeShield()};
    const auto entities = fixture.runtime.usertype<rtype::ecs::Entity>();
    const auto first = entities.create(rtype::ecs::Entity{1, 2});
    const auto same = entities.create(rtype::ecs::Entity{1, 2});
    const auto other = entities.create(rtype::ecs::Entity{1, 3});
    ASSERT_TRUE(first && same && other);
    ASSERT_TRUE(fixture.runtime.globals().set("first", rtype::luau::Value{*first}));
    ASSERT_TRUE(fixture.runtime.globals().set("same", rtype::luau::Value{*same}));
    ASSERT_TRUE(fixture.runtime.globals().set("other", rtype::luau::Value{*other}));

    ASSERT_EQ(
        fixture.run("equal = first == same; different = first ~= other; text = tostring(first); key = first.bits"), "");

    EXPECT_TRUE(*fixture.runtime.globals().get("equal")->tryAs<bool>());
    EXPECT_TRUE(*fixture.runtime.globals().get("different")->tryAs<bool>());
    EXPECT_EQ(*fixture.runtime.globals().get("text")->tryAs<std::string>(), "Entity(1:2)");
    EXPECT_EQ(fixture.number("key"), static_cast<double>((2ULL << 32U) | 1ULL));
}

TEST(FillFromTable, WritesTheNamedFieldsAndKeepsTheOthers) {
    Fixture fixture{makeShield()};
    auto table = rtype::luau::Table::create(fixture.runtime);
    ASSERT_TRUE(table);
    ASSERT_TRUE(table->set("strength", rtype::luau::Value{10.0}));
    const float regen = 2.0F;
    for (const auto& field : fixture.info.type.fields) {
        if (field.name == "regen") {
            std::memcpy(fixture.bytes.data() + field.offset, &regen, sizeof(regen));
        }
    }

    const auto result = sv::fillFromTable(fixture.info.type, *table, fixture.bytes);

    ASSERT_TRUE(result);
    EXPECT_EQ(fixture.read<float>("strength"), 10.0F);
    EXPECT_EQ(fixture.read<float>("regen"), 2.0F);
}

TEST(FillFromTable, RefusesAnUnknownFieldAndABadValue) {
    Fixture fixture{makeShield()};
    auto unknown = rtype::luau::Table::create(fixture.runtime);
    ASSERT_TRUE(unknown);
    ASSERT_TRUE(unknown->set("bogus", rtype::luau::Value{1.0}));
    auto wrong = rtype::luau::Table::create(fixture.runtime);
    ASSERT_TRUE(wrong);
    ASSERT_TRUE(wrong->set("strength", rtype::luau::Value{std::string{"high"}}));

    const auto unknownResult = sv::fillFromTable(fixture.info.type, *unknown, fixture.bytes);
    const auto wrongResult = sv::fillFromTable(fixture.info.type, *wrong, fixture.bytes);

    ASSERT_FALSE(unknownResult);
    EXPECT_EQ(unknownResult.error().kind(), rtype::scripting::ErrorKind::kInvalidValue);
    EXPECT_NE(unknownResult.error().message().find("bogus"), std::string_view::npos);
    ASSERT_FALSE(wrongResult);
    EXPECT_NE(wrongResult.error().message().find("strength"), std::string_view::npos);
}
