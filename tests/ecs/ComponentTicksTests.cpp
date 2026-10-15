/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentTicksTests
*/

#include <gtest/gtest.h>

#include "rtype/ecs/change/ComponentTicks.hpp"

TEST(ComponentTicks, AValueAddedAfterTheLastRunIsAddedAndChanged) {
    const rtype::ecs::ComponentTicks ticks{.added = 7, .changed = 7};
    EXPECT_TRUE(ticks.isAdded(5, 10));
    EXPECT_TRUE(ticks.isChanged(5, 10));
}

TEST(ComponentTicks, AValueWrittenAfterTheLastRunIsChangedButNotAdded) {
    const rtype::ecs::ComponentTicks ticks{.added = 2, .changed = 7};
    EXPECT_FALSE(ticks.isAdded(5, 10));
    EXPECT_TRUE(ticks.isChanged(5, 10));
}

TEST(ComponentTicks, AnUntouchedValueIsNeither) {
    const rtype::ecs::ComponentTicks ticks{.added = 2, .changed = 3};
    EXPECT_FALSE(ticks.isAdded(5, 10));
    EXPECT_FALSE(ticks.isChanged(5, 10));
}
