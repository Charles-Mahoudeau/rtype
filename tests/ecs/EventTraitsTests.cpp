/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventTraitsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/type/NameHash.hpp"
#include "rtype/ecs/type/TypeTraits.hpp"

namespace rtype::test::event_traits {
struct Collision {
    std::uint32_t a;
    std::uint32_t b;
};
RTYPE_ECS_EVENT(Collision, "test.traits.Collision", a, b);

/// @brief A move-only event, like one carrying a buffer it owns.
struct Packet {
    std::unique_ptr<std::int32_t> payload;
};
RTYPE_ECS_EVENT(Packet, "test.traits.Packet");

struct GameOver {};
RTYPE_ECS_EVENT(GameOver, "test.traits.GameOver");

struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.traits.Health", value);

struct Time {
    float delta;
};
RTYPE_ECS_RESOURCE(Time, "test.traits.Time");
}  // namespace rtype::test::event_traits

namespace evt = rtype::test::event_traits;

TEST(EventTraits, ConceptOnlyAcceptsEvents) {
    static_assert(rtype::ecs::Event<evt::Collision>);
    static_assert(rtype::ecs::Event<evt::Packet>);
    static_assert(rtype::ecs::Event<evt::GameOver>);
    static_assert(!rtype::ecs::Event<evt::Health>);
    static_assert(!rtype::ecs::Event<evt::Time>);
    static_assert(!rtype::ecs::Event<std::int32_t>);
    static_assert(!rtype::ecs::Component<evt::Collision>);
    static_assert(!rtype::ecs::Resource<evt::Collision>);
}

TEST(EventTraits, RecordsTheNameKindAndHash) {
    static_assert(rtype::ecs::EventTraits<evt::Collision>::kName == "test.traits.Collision");
    static_assert(rtype::ecs::EventTraits<evt::Collision>::kKind == rtype::ecs::TypeKind::kEvent);
    static_assert(rtype::ecs::EventTraits<evt::Collision>::kHash == rtype::ecs::hashName("test.traits.Collision"));
}

TEST(EventTraits, MakesTheDescription) {
    const rtype::ecs::EventInfo info = rtype::ecs::EventTraits<evt::Collision>::makeInfo();
    EXPECT_EQ(info.type.id, rtype::ecs::kInvalidEventId);
    EXPECT_EQ(info.type.name, "test.traits.Collision");
    EXPECT_EQ(info.type.size, sizeof(evt::Collision));
    EXPECT_EQ(info.type.alignment, alignof(evt::Collision));
    EXPECT_TRUE(info.type.copyable);
    ASSERT_EQ(info.type.fields.size(), 2U);
    EXPECT_EQ(info.type.fields[0].name, "a");
    EXPECT_EQ(info.type.fields[1].name, "b");
}

TEST(EventTraits, AcceptsMoveOnlyEvents) {
    const rtype::ecs::EventInfo info = rtype::ecs::EventTraits<evt::Packet>::makeInfo();
    EXPECT_FALSE(info.type.copyable);
    EXPECT_NE(info.type.hooks.move, nullptr);
    EXPECT_NE(info.type.hooks.destruct, nullptr);
    EXPECT_EQ(info.type.hooks.copy, nullptr);
}

TEST(EventTraits, AnEventWithoutDataHasSizeZero) {
    EXPECT_EQ(rtype::ecs::EventTraits<evt::GameOver>::makeInfo().type.size, 0U);
}
