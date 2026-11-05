/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicEventsTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <span>
#include <vector>

#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/exceptions/TypeExceptions.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/system/DynamicEvents.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::dynamic_events {
struct Boom {
    int power;
};
RTYPE_ECS_EVENT(Boom, "test.dynamicEvents.Boom", power);

/// @brief What the system under test read.
struct Heard {
    int sum;
    int count;
};
RTYPE_ECS_RESOURCE(Heard, "test.dynamicEvents.Heard", sum, count);
}  // namespace rtype::test::dynamic_events

namespace {
using rtype::ecs::DynamicEvents;
using rtype::ecs::EventCursor;
using rtype::ecs::EventId;
using rtype::ecs::EventList;
using rtype::ecs::ResMut;
using rtype::ecs::System;
using rtype::ecs::World;
using rtype::test::dynamic_events::Boom;
using rtype::test::dynamic_events::Heard;

/// @return The int at the start of `bytes`.
int intOf(std::span<const std::byte> bytes) {
    int value = 0;
    std::memcpy(&value, bytes.data(), sizeof(int));
    return value;
}

/// @brief Adds the power of every event of type 0 to Heard.
void listen(DynamicEvents events, ResMut<Heard> heard) {
    for (const std::span<const std::byte> bytes : events.read(0)) {
        heard->sum += intOf(bytes);
        ++heard->count;
    }
}

/// @brief Sends a Boom of power 7 through type 0.
void shout(DynamicEvents events) {
    const int power = 7;
    static_cast<void>(events.send(0, std::as_bytes(std::span<const int>{&power, 1})));
}

class DynamicEventsTest : public ::testing::Test {
  protected:
    void SetUp() override { world.getResources().insert(Heard{.sum = 0, .count = 0}); }

    /// @return What the systems heard.
    [[nodiscard]] const Heard& heard() const { return world.getResources().require<Heard>(); }

    World world;                                           ///< The world under test.
    EventId boom{world.getEvents().registerType<Boom>()};  ///< Boom's id.
};
}  // namespace

TEST_F(DynamicEventsTest, ReadsEventsSentFromCpp) {
    world.getEvents().send(Boom{.power = 3});
    world.getEvents().send(Boom{.power = 4});
    System system{world, "test.listen", &listen, EventList{{{.id = boom, .read = true, .send = false}}}};
    system.run();
    EXPECT_EQ(heard().sum, 7);
    EXPECT_EQ(heard().count, 2);
}

TEST_F(DynamicEventsTest, ReadsEachEventOnce) {
    world.getEvents().send(Boom{.power = 3});
    System system{world, "test.listen", &listen, EventList{{{.id = boom, .read = true, .send = false}}}};
    system.run();
    system.run();
    EXPECT_EQ(heard().count, 1);
}

TEST_F(DynamicEventsTest, SendsEventsCppReads) {
    System system{world, "test.shout", &shout, EventList{{{.id = boom, .read = false, .send = true}}}};
    system.run();
    EventCursor cursor;
    const auto range = world.getEvents().read<Boom>(cursor);
    ASSERT_EQ(range.size(), 1U);
    EXPECT_EQ((*range.begin()).power, 7);
}

TEST_F(DynamicEventsTest, IgnoresWhatWasNotDeclared) {
    world.getEvents().send(Boom{.power = 3});
    System silent{world, "test.shout", &shout, EventList{{{.id = boom, .read = true, .send = false}}}};
    silent.run();  // declared read: sending does nothing
    EXPECT_EQ(world.getEvents().getCount(boom), 1U);
    System deaf{world, "test.listen", &listen, EventList{{{.id = boom, .read = false, .send = true}}}};
    deaf.run();  // declared sent: reading gives nothing
    EXPECT_EQ(heard().count, 0);
}

TEST_F(DynamicEventsTest, DeclaresItsAccess) {
    const System reader{world, "test.listen", &listen, EventList{{{.id = boom, .read = true, .send = false}}}};
    const System writer{world, "test.shout", &shout, EventList{{{.id = boom, .read = false, .send = true}}}};
    ASSERT_EQ(reader.getAccess().getEvents().size(), 1U);
    EXPECT_EQ(reader.getAccess().getEvents().front().id, boom);
    EXPECT_FALSE(reader.getAccess().getEvents().front().write);
    EXPECT_TRUE(writer.getAccess().getEvents().front().write);
}

TEST_F(DynamicEventsTest, RefusesAnUnknownEvent) {
    EXPECT_THROW((System{world, "test.listen", &listen, EventList{{{.id = 999, .read = true, .send = false}}}}),
                 rtype::ecs::exceptions::TypeRegistrationException);
}
