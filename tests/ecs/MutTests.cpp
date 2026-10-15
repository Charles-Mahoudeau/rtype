/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** MutTests
*/

#include <gtest/gtest.h>

#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/change/Tick.hpp"

namespace {
struct Health {
    int value;
};
}  // namespace

TEST(Mut, ReadingDoesNotMarkChanged) {
    Health health{.value = 3};
    rtype::ecs::Tick changed = 1;
    const rtype::ecs::Mut<Health> mut{&health, &changed, 9};
    EXPECT_EQ(mut.get().value, 3);
    EXPECT_EQ(changed, 1U);
}

TEST(Mut, ArrowMarksChangedAndWrites) {
    Health health{.value = 3};
    rtype::ecs::Tick changed = 1;
    rtype::ecs::Mut<Health> mut{&health, &changed, 9};
    mut->value = 4;
    EXPECT_EQ(health.value, 4);
    EXPECT_EQ(changed, 9U);
}

TEST(Mut, DereferenceMarksChangedAndWrites) {
    Health health{.value = 3};
    rtype::ecs::Tick changed = 1;
    rtype::ecs::Mut<Health> mut{&health, &changed, 9};
    (*mut).value = 5;
    EXPECT_EQ(health.value, 5);
    EXPECT_EQ(changed, 9U);
}

TEST(Mut, ArrowMarksEvenWhenOnlyReading) {
    Health health{.value = 3};
    rtype::ecs::Tick changed = 1;
    rtype::ecs::Mut<Health> mut{&health, &changed, 9};
    const int value = mut->value;  // reads, but through write access: use get() to avoid this
    EXPECT_EQ(value, 3);
    EXPECT_EQ(changed, 9U);
}

TEST(Mut, ADefaultHandleIsEmpty) {
    const rtype::ecs::Mut<Health> mut;
    EXPECT_FALSE(mut);
    Health health{.value = 0};
    rtype::ecs::Tick changed = 0;
    EXPECT_TRUE((rtype::ecs::Mut<Health>{&health, &changed, 1}));
}

TEST(Mut, CopiesShareTheValue) {
    Health health{.value = 3};
    rtype::ecs::Tick changed = 1;
    const rtype::ecs::Mut<Health> original{&health, &changed, 9};
    rtype::ecs::Mut<Health> copy = original;
    copy->value = 8;
    EXPECT_EQ(original.get().value, 8);
}
