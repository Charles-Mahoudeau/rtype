/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventRegistryTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventRegistry.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/exceptions/TypeExceptions.hpp"
#include "rtype/ecs/type/NameHash.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::test::event_registry {
struct Collision {
    std::uint32_t a;
    std::uint32_t b;
};
RTYPE_ECS_EVENT(Collision, "test.registry.Collision", a, b);

struct Resized {
    std::int32_t width;
    std::int32_t height;
};
RTYPE_ECS_EVENT(Resized, "test.registry.Resized", width, height);

/// @brief Another layout under Collision's name.
struct OtherCollision {
    std::uint64_t pair;
};
RTYPE_ECS_EVENT(OtherCollision, "test.registry.Collision", pair);
}  // namespace rtype::test::event_registry

namespace evr = rtype::test::event_registry;

TEST(EventRegistry, GivesDenseIdsFromZero) {
    rtype::ecs::EventRegistry registry;
    EXPECT_EQ(registry.registerType<evr::Collision>(), 0U);
    EXPECT_EQ(registry.registerType<evr::Resized>(), 1U);
    EXPECT_EQ(registry.getCount(), 2U);
}

TEST(EventRegistry, RegisteringTheSameEventTwiceReturnsTheSameId) {
    rtype::ecs::EventRegistry registry;
    const rtype::ecs::EventId id = registry.registerType<evr::Collision>();
    EXPECT_EQ(registry.registerType<evr::Collision>(), id);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(EventRegistry, FindsEventsByIdNameHashAndType) {
    rtype::ecs::EventRegistry registry;
    const rtype::ecs::EventId id = registry.registerType<evr::Collision>();
    const rtype::ecs::EventInfo* info = registry.findById(id);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(registry.findByName("test.registry.Collision"), info);
    EXPECT_EQ(registry.findByHash(rtype::ecs::hashName("test.registry.Collision")), info);
    EXPECT_EQ(registry.find<evr::Collision>(), info);
    EXPECT_EQ(registry.find<evr::Resized>(), nullptr);
}

TEST(EventRegistry, AppliesThePresence) {
    rtype::ecs::EventRegistry registry;
    registry.registerType<evr::Resized>({.presence = rtype::ecs::Presence::kClientOnly});
    EXPECT_EQ(registry.find<evr::Resized>()->type.presence, rtype::ecs::Presence::kClientOnly);
}

TEST(EventRegistry, RefusesAnotherPresenceForTheSameName) {
    rtype::ecs::EventRegistry registry;
    registry.registerType<evr::Resized>({.presence = rtype::ecs::Presence::kClientOnly});
    EXPECT_THROW(registry.registerType<evr::Resized>({.presence = rtype::ecs::Presence::kServerOnly}),
                 rtype::ecs::exceptions::TypeRegistrationException);
}

TEST(EventRegistry, RefusesTheSameNameWithAnotherLayout) {
    rtype::ecs::EventRegistry registry;
    registry.registerType<evr::Collision>();
    EXPECT_THROW(registry.registerType<evr::OtherCollision>(), rtype::ecs::exceptions::TypeRegistrationException);
    EXPECT_EQ(registry.find<evr::OtherCollision>(), nullptr);
}

TEST(EventRegistry, RefusesAReplicatedRunTimeEvent) {
    rtype::ecs::EventRegistry registry;
    rtype::ecs::EventInfo info = rtype::ecs::EventTraits<evr::Collision>::makeInfo();
    info.type.replication = rtype::ecs::ReplicationMode::kEveryChange;
    const rtype::ecs::Result<rtype::ecs::EventId> id = registry.registerType(info);
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kInvalidType);
    EXPECT_EQ(registry.getCount(), 0U);
}
