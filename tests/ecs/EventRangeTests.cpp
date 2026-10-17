/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventRangeTests
*/

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventRange.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::event_range {
struct Collision {
    std::uint32_t a;
    std::uint32_t b;
};
RTYPE_ECS_EVENT(Collision, "test.range.Collision", a, b);
}  // namespace rtype::test::event_range

namespace evg = rtype::test::event_range;

static_assert(std::forward_iterator<rtype::ecs::RawEventRange::Iterator>);
static_assert(std::forward_iterator<rtype::ecs::EventRange<evg::Collision>::Iterator>);
static_assert(std::ranges::forward_range<rtype::ecs::RawEventRange>);
static_assert(std::ranges::forward_range<rtype::ecs::EventRange<evg::Collision>>);

namespace {
/// @brief A world with three collisions sent, (0, 10), (1, 11) and (2, 12).
rtype::ecs::World makeWorld() {
    rtype::ecs::World world;
    for (std::uint32_t index = 0; index < 3; ++index) {
        world.getEvents().send(evg::Collision{.a = index, .b = index + 10});
    }
    return world;
}
}  // namespace

TEST(EventRange, PostIncrementReturnsThePreviousPosition) {
    const rtype::ecs::World world = makeWorld();
    rtype::ecs::EventCursor cursor;
    const rtype::ecs::EventRange<evg::Collision> collisions = world.getEvents().read<evg::Collision>(cursor);
    auto it = collisions.begin();
    const auto before = it++;
    EXPECT_EQ(before->a, 0U);
    EXPECT_EQ(it->a, 1U);
}

TEST(EventRange, WorksWithStandardAlgorithms) {
    const rtype::ecs::World world = makeWorld();
    rtype::ecs::EventCursor cursor;
    const rtype::ecs::EventRange<evg::Collision> collisions = world.getEvents().read<evg::Collision>(cursor);
    EXPECT_EQ(std::ranges::distance(collisions), 3);
    EXPECT_TRUE(std::ranges::any_of(collisions, [](const evg::Collision& collision) { return collision.b == 12U; }));
    std::vector<std::uint32_t> firsts;
    std::ranges::transform(collisions, std::back_inserter(firsts),
                           [](const evg::Collision& collision) { return collision.a; });
    EXPECT_EQ(firsts, (std::vector<std::uint32_t>{0, 1, 2}));
}

TEST(EventRange, CanBeWalkedSeveralTimes) {
    const rtype::ecs::World world = makeWorld();
    rtype::ecs::EventCursor cursor;
    const rtype::ecs::EventRange<evg::Collision> collisions = world.getEvents().read<evg::Collision>(cursor);
    EXPECT_EQ(std::ranges::distance(collisions), 3);
    EXPECT_EQ(std::ranges::distance(collisions), 3);
}

TEST(EventRange, RawRangeWorksWithStandardAlgorithms) {
    const rtype::ecs::World world = makeWorld();
    const rtype::ecs::EventId id = *world.getEvents().find("test.range.Collision");
    rtype::ecs::EventCursor cursor;
    const rtype::ecs::RawEventRange events = world.getEvents().read(id, cursor);
    EXPECT_EQ(std::ranges::distance(events), 3);
    EXPECT_TRUE(std::ranges::all_of(
        events, [](std::span<const std::byte> bytes) { return bytes.size() == sizeof(evg::Collision); }));
    auto it = events.begin();
    it++;
    EXPECT_EQ(rtype::ecs::storage::viewAs<evg::Collision>(*it)->a, 1U);
}

TEST(EventRange, DefaultIteratorsAreEqual) {
    EXPECT_EQ(rtype::ecs::RawEventRange::Iterator{}, rtype::ecs::RawEventRange::Iterator{});
    EXPECT_EQ(rtype::ecs::EventRange<evg::Collision>::Iterator{}, rtype::ecs::EventRange<evg::Collision>::Iterator{});
}
