/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NameHashTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "core/hash/NameHash.hpp"

static_assert(rtype::core::hash::hashName("rtype.Health") != rtype::core::hash::hashName("rtype.Position"));

TEST(NameHash, MatchesTheFnv1aReferenceValues) {
    EXPECT_EQ(rtype::core::hash::hashName(""), 0xcbf29ce484222325ULL);
    EXPECT_EQ(rtype::core::hash::hashName("a"), 0xaf63dc4c8601ec8cULL);
}

TEST(NameHash, DependsOnEveryCharacter) {
    EXPECT_NE(rtype::core::hash::hashName("rtype.Health"), rtype::core::hash::hashName("rtype.health"));
    EXPECT_NE(rtype::core::hash::hashName("ab"), rtype::core::hash::hashName("ba"));
    EXPECT_EQ(rtype::core::hash::hashName("rtype.Health"), rtype::core::hash::hashName("rtype.Health"));
}

TEST(NameHash, IsTheStringStepFromTheOffsetBasis) {
    EXPECT_EQ(rtype::core::hash::hashName("rtype.Health"),
              rtype::core::hash::hashString(rtype::core::hash::kNameHashOffsetBasis, "rtype.Health"));
}

TEST(NameHash, IntegerStepDependsOnEveryByte) {
    const std::uint64_t start = rtype::core::hash::kNameHashOffsetBasis;
    EXPECT_NE(rtype::core::hash::hashInteger(start, 1), rtype::core::hash::hashInteger(start, 2));
    EXPECT_NE(rtype::core::hash::hashInteger(start, 0x0100000000000000ULL), rtype::core::hash::hashInteger(start, 0));
    EXPECT_EQ(rtype::core::hash::hashInteger(start, 42), rtype::core::hash::hashInteger(start, 42));
}
