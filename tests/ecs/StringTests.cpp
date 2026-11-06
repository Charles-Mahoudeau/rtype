/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** StringTests
*/

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <type_traits>

#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/FieldTypeOf.hpp"
#include "rtype/ecs/type/String.hpp"

namespace {
using rtype::ecs::String;
}  // namespace

TEST(String, StartsEmpty) {
    const String text;

    EXPECT_TRUE(text.empty());
    EXPECT_EQ(text.size(), 0U);
    EXPECT_EQ(text.view(), "");
}

TEST(String, ConvertsFromTextsAndBack) {
    const String fromView{std::string_view{"view"}};
    const String fromLiteral = "literal";
    const String fromString{std::string{"string"}};

    EXPECT_EQ(fromView.view(), "view");
    EXPECT_EQ(fromLiteral.view(), "literal");
    EXPECT_EQ(fromString.view(), "string");
    const std::string_view back = fromLiteral;
    EXPECT_EQ(back, "literal");
}

TEST(String, AssignsANewText) {
    String text{"first"};

    text = "second";

    EXPECT_EQ(text.view(), "second");
    EXPECT_EQ(text.size(), 6U);
}

TEST(String, ShorterTextLeavesNoOldBytes) {
    String text{"a long text"};
    text = "ab";

    EXPECT_EQ(text, String{"ab"});
}

TEST(String, KeepsATextOfExactlyItsCapacity) {
    const std::string full(String::kCapacity, 'x');

    EXPECT_EQ(String{full}.size(), String::kCapacity);
}

TEST(String, CutsALongerText) {
    const std::string tooLong(String::kCapacity + 10, 'x');

    EXPECT_EQ(String{tooLong}.size(), String::kCapacity);
}

TEST(String, CutsAtACharacterBoundary) {
    // "é" takes two bytes: one at the capacity, so it does not fit and the whole character is dropped.
    const std::string text = std::string(String::kCapacity - 1, 'x') + "é" + "y";

    const String cut{text};

    EXPECT_EQ(cut.size(), String::kCapacity - 1);
    EXPECT_EQ(cut.view(), std::string(String::kCapacity - 1, 'x'));
}

TEST(String, ComparesByText) {
    EXPECT_EQ(String{"same"}, String{"same"});
    EXPECT_NE(String{"one"}, String{"two"});
}

TEST(String, IsPlainDataOfTheSizeOfItsField) {
    EXPECT_TRUE(std::is_trivially_copyable_v<String>);
    EXPECT_EQ(sizeof(String), rtype::ecs::getFieldLayout(rtype::ecs::FieldType::kString).size);
    EXPECT_EQ(alignof(String), rtype::ecs::getFieldLayout(rtype::ecs::FieldType::kString).alignment);
    EXPECT_EQ(rtype::ecs::FieldTypeOf<String>::value, rtype::ecs::FieldType::kString);
}
