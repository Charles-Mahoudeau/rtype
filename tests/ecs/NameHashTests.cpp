/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** NameHashTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "rtype/ecs/component/NameHash.hpp"

static_assert(rtype::ecs::hashName("rtype.Health") != rtype::ecs::hashName("rtype.Position"));

TEST(NameHash, MatchesTheFnv1aReferenceValues) {
    EXPECT_EQ(rtype::ecs::hashName(""), 0xcbf29ce484222325ULL);
    EXPECT_EQ(rtype::ecs::hashName("a"), 0xaf63dc4c8601ec8cULL);
}

TEST(NameHash, DependsOnEveryCharacter) {
    EXPECT_NE(rtype::ecs::hashName("rtype.Health"), rtype::ecs::hashName("rtype.health"));
    EXPECT_NE(rtype::ecs::hashName("ab"), rtype::ecs::hashName("ba"));
    EXPECT_EQ(rtype::ecs::hashName("rtype.Health"), rtype::ecs::hashName("rtype.Health"));
}
