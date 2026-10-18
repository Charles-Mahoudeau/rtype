/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventParamTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <iterator>
#include <ranges>
#include <vector>

#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/system/EventReader.hpp"
#include "rtype/ecs/system/EventWriter.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::event_param {
struct Collision {
    std::uint32_t a;
    std::uint32_t b;
};
RTYPE_ECS_EVENT(Collision, "test.event_param.Collision", a, b);
}  // namespace rtype::test::event_param

namespace {
using rtype::ecs::EventCursor;
using rtype::ecs::EventReader;
using rtype::ecs::EventWriter;
using rtype::ecs::World;
using rtype::test::event_param::Collision;

static_assert(std::ranges::forward_range<EventReader<Collision>>);

/// @return The `a` of every event a reader took, in order.
std::vector<std::uint32_t> firstsOf(const EventReader<Collision>& reader) {
    std::vector<std::uint32_t> firsts;
    for (const Collision& collision : reader) {
        firsts.push_back(collision.a);
    }
    return firsts;
}
}  // namespace

TEST(EventWriter, SendsEvents) {
    World world;
    EventWriter<Collision> writer{world.getEvents()};
    writer.send(Collision{.a = 1, .b = 2});
    writer.send(Collision{.a = 3, .b = 4});
    EXPECT_EQ(world.getEvents().getCount<Collision>(), 2U);
}

TEST(EventReader, TakesTheEventsNotReadYet) {
    World world;
    EventWriter<Collision> writer{world.getEvents()};
    writer.send(Collision{.a = 1, .b = 0});
    writer.send(Collision{.a = 2, .b = 0});
    EventCursor cursor;

    const EventReader<Collision> first{world.getEvents(), cursor};
    EXPECT_EQ(first.size(), 2U);
    EXPECT_EQ(firstsOf(first), (std::vector<std::uint32_t>{1, 2}));
    EXPECT_EQ(firstsOf(first), (std::vector<std::uint32_t>{1, 2}));  // a range: walked again

    writer.send(Collision{.a = 3, .b = 0});
    const EventReader<Collision> second{world.getEvents(), cursor};  // the next run
    EXPECT_EQ(firstsOf(second), (std::vector<std::uint32_t>{3}));    // each event once
    const EventReader<Collision> third{world.getEvents(), cursor};
    EXPECT_TRUE(third.empty());
}

TEST(EventReader, SeesEventsSentAfterItRanInItsNextRun) {
    World world;
    EventCursor cursor;
    const EventReader<Collision> before{world.getEvents(), cursor};  // runs before the writer
    EXPECT_TRUE(before.empty());
    EventWriter<Collision>{world.getEvents()}.send(Collision{.a = 7, .b = 0});
    world.getEvents().update();  // end of the update
    const EventReader<Collision> after{world.getEvents(), cursor};
    EXPECT_EQ(firstsOf(after), (std::vector<std::uint32_t>{7}));
}

TEST(EventReader, ReadersWithTheirOwnCursorsAreIndependent) {
    World world;
    EventWriter<Collision>{world.getEvents()}.send(Collision{.a = 5, .b = 0});
    EventCursor damage;
    EventCursor sound;
    EXPECT_EQ((EventReader<Collision>{world.getEvents(), damage}.size()), 1U);
    EXPECT_EQ((EventReader<Collision>{world.getEvents(), sound}.size()), 1U);
}
