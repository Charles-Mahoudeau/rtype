/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SystemAccessTests
*/

#include <gtest/gtest.h>

#include <span>
#include <vector>

#include "rtype/ecs/system/SystemAccess.hpp"

namespace {
using rtype::ecs::SystemAccess;
using rtype::ecs::TypeAccess;

/// @return The accesses as a vector, to compare them.
std::vector<TypeAccess> toVector(std::span<const TypeAccess> accesses) { return {accesses.begin(), accesses.end()}; }
}  // namespace

TEST(SystemAccess, StartsEmpty) {
    const SystemAccess access;
    EXPECT_TRUE(access.getComponents().empty());
    EXPECT_TRUE(access.getResources().empty());
    EXPECT_TRUE(access.getEvents().empty());
    EXPECT_FALSE(access.isMainThread());
}

TEST(SystemAccess, KeepsEachKindApart) {
    SystemAccess access;
    access.addComponent(3, true);
    access.addResource(3, false);
    access.addEvent(3, true);
    EXPECT_EQ(toVector(access.getComponents()), (std::vector<TypeAccess>{{.id = 3, .write = true}}));
    EXPECT_EQ(toVector(access.getResources()), (std::vector<TypeAccess>{{.id = 3, .write = false}}));
    EXPECT_EQ(toVector(access.getEvents()), (std::vector<TypeAccess>{{.id = 3, .write = true}}));
}

TEST(SystemAccess, ListsEachTypeOnceWithWriteWinning) {
    SystemAccess access;
    access.addComponent(1, false);
    access.addComponent(2, true);
    access.addComponent(1, true);   // a read becomes a write
    access.addComponent(2, false);  // a write stays a write
    EXPECT_EQ(toVector(access.getComponents()),
              (std::vector<TypeAccess>{{.id = 1, .write = true}, {.id = 2, .write = true}}));
}

TEST(SystemAccess, RecordsTheMainThreadNeed) {
    SystemAccess access;
    access.requireMainThread();
    EXPECT_TRUE(access.isMainThread());
}
