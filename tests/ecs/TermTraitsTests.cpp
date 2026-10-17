/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TermTraitsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <type_traits>

#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/DynamicQuery.hpp"
#include "rtype/ecs/query/QueryTerm.hpp"
#include "rtype/ecs/query/QueryTerms.hpp"
#include "rtype/ecs/query/TermTraits.hpp"
#include "rtype/ecs/storage/SparseSetStorage.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::term_traits {
struct Position {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Position, "test.term_traits.Position", x, y);

struct Velocity {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Velocity, "test.term_traits.Velocity", x, y);

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "test.term_traits.Dead");

/// @brief Not a component: no RTYPE_ECS_COMPONENT.
struct Plain {
    int value;
};
}  // namespace rtype::test::term_traits

namespace {
using rtype::ecs::Added;
using rtype::ecs::Changed;
using rtype::ecs::Mut;
using rtype::ecs::Optional;
using rtype::ecs::TermKind;
using rtype::ecs::TermTraits;
using rtype::ecs::ValidQuery;
using rtype::ecs::With;
using rtype::ecs::Without;
using rtype::test::term_traits::Dead;
using rtype::test::term_traits::Plain;
using rtype::test::term_traits::Position;
using rtype::test::term_traits::Velocity;

// Each C++ term maps to its dynamic kind and yields the documented item.
static_assert(TermTraits<Position>::kKind == TermKind::kWrite);
static_assert(std::is_same_v<TermTraits<Position>::Item, Mut<Position>>);
static_assert(TermTraits<const Position>::kKind == TermKind::kRead);
static_assert(std::is_same_v<TermTraits<const Position>::Item, const Position&>);
static_assert(TermTraits<Optional<Position>>::kKind == TermKind::kOptionalWrite);
static_assert(std::is_same_v<TermTraits<Optional<Position>>::Item, Mut<Position>>);
static_assert(TermTraits<Optional<const Position>>::kKind == TermKind::kOptionalRead);
static_assert(std::is_same_v<TermTraits<Optional<const Position>>::Item, const Position*>);
static_assert(TermTraits<With<Dead>>::kKind == TermKind::kWith && !TermTraits<With<Dead>>::kHasItem);
static_assert(TermTraits<Without<Dead>>::kKind == TermKind::kWithout && !TermTraits<Without<Dead>>::kHasItem);
static_assert(TermTraits<Added<Position>>::kKind == TermKind::kAdded && !TermTraits<Added<Position>>::kHasItem);
static_assert(TermTraits<Changed<Position>>::kKind == TermKind::kChanged);
static_assert(std::is_same_v<TermTraits<const Velocity>::Type, Velocity>);

// Only components, in the documented wrappers, are terms.
static_assert(rtype::ecs::TypedTerm<Position>);
static_assert(rtype::ecs::TypedTerm<Optional<const Position>>);
static_assert(!rtype::ecs::TypedTerm<Plain>);
static_assert(!rtype::ecs::TypedTerm<With<Plain>>);
static_assert(!rtype::ecs::TypedTerm<Position*>);
static_assert(!rtype::ecs::TypedTerm<Position&>);

// Valid queries.
static_assert(ValidQuery<Position, const Velocity>);
static_assert(ValidQuery<Position, const Velocity, Without<Dead>>);
static_assert(ValidQuery<const Position, Optional<Velocity>>);
static_assert(ValidQuery<With<Dead>>);
static_assert(ValidQuery<Position, Changed<Position>>);  // write the values that changed
static_assert(ValidQuery<const Position, With<Position>>);
static_assert(ValidQuery<Added<Position>, Changed<Position>>);

// Invalid queries (#116: two writable terms of the same type are a compile error).
static_assert(!ValidQuery<>);
static_assert(!ValidQuery<Position, Position>);
static_assert(!ValidQuery<Position, const Position>);
static_assert(!ValidQuery<const Position, Optional<Position>>);
static_assert(!ValidQuery<const Position, Without<Position>>);
static_assert(!ValidQuery<With<Dead>, Without<Dead>>);
static_assert(!ValidQuery<Optional<Position>>);  // optional only: no required term
static_assert(!ValidQuery<Without<Dead>>);
static_assert(!ValidQuery<Position, Changed<Velocity>, Changed<Velocity>>);
static_assert(!ValidQuery<Dead>);  // a tag has no value: use With<Dead>
static_assert(!ValidQuery<const Dead, Position>);
static_assert(!ValidQuery<Position, Plain>);
}  // namespace

TEST(QueryTerm, FindsConflictsBetweenTwoTermsOnOneComponent) {
    using rtype::ecs::findConflict;
    using rtype::ecs::TermConflict;
    EXPECT_EQ(findConflict(TermKind::kWrite, TermKind::kWrite), TermConflict::kSameTerm);
    EXPECT_EQ(findConflict(TermKind::kRead, TermKind::kOptionalWrite), TermConflict::kTwoDataTerms);
    EXPECT_EQ(findConflict(TermKind::kWith, TermKind::kWithout), TermConflict::kExcluded);
    EXPECT_EQ(findConflict(TermKind::kWithout, TermKind::kRead), TermConflict::kExcluded);
    EXPECT_EQ(findConflict(TermKind::kWrite, TermKind::kChanged), TermConflict::kNone);
    EXPECT_EQ(findConflict(TermKind::kRead, TermKind::kWith), TermConflict::kNone);
    EXPECT_EQ(findConflict(TermKind::kAdded, TermKind::kChanged), TermConflict::kNone);
}

TEST(TermTraits, FetchesAWritableValueThatStampsOnlyOnWrite) {
    rtype::ecs::World world;
    const rtype::ecs::Entity ship = world.spawn(Position{.x = 1.F, .y = 2.F});
    rtype::ecs::Components& components = world.getComponents();
    rtype::ecs::storage::SparseSetStorage& pool = *components.findPool(components.registerType<Position>());

    Mut<Position> position = TermTraits<Position>::fetch(pool, 0, 9);
    EXPECT_EQ(position.get().x, 1.F);
    EXPECT_EQ(components.getTicks<Position>(ship)->changed, 0U);  // reading through get() does not stamp
    position->x = 5.F;
    EXPECT_EQ(components.get<Position>(ship)->x, 5.F);
    EXPECT_EQ(components.getTicks<Position>(ship)->changed, 9U);
}

TEST(TermTraits, FetchesAReadOnlyValue) {
    rtype::ecs::World world;
    world.spawn(Velocity{.x = 3.F, .y = 4.F});
    rtype::ecs::Components& components = world.getComponents();
    rtype::ecs::storage::SparseSetStorage& pool = *components.findPool(components.registerType<Velocity>());

    const Velocity& velocity = TermTraits<const Velocity>::fetch(pool, 0, 9);
    EXPECT_EQ(velocity.y, 4.F);
}

TEST(TermTraits, FetchesAnEmptyOptionalForAnAbsentComponent) {
    rtype::ecs::World world;
    world.spawn(Position{.x = 1.F, .y = 2.F});
    rtype::ecs::Components& components = world.getComponents();
    rtype::ecs::storage::SparseSetStorage& pool = *components.findPool(components.registerType<Position>());
    constexpr std::uint32_t kAbsent = rtype::ecs::DynamicQuery::kNoPosition;

    EXPECT_FALSE(TermTraits<Optional<Position>>::fetch(pool, kAbsent, 9));
    EXPECT_EQ(TermTraits<Optional<const Position>>::fetch(pool, kAbsent, 9), nullptr);
    EXPECT_TRUE(TermTraits<Optional<Position>>::fetch(pool, 0, 9));
    const Position* present = TermTraits<Optional<const Position>>::fetch(pool, 0, 9);
    ASSERT_NE(present, nullptr);
    EXPECT_EQ(present->y, 2.F);
}
