/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EventInfoTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <memory>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/event/EventInfo.hpp"
#include "rtype/ecs/event/EventTraits.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace rtype::test::event_info {
struct Hit {
    std::int32_t damage;
};
RTYPE_ECS_EVENT(Hit, "test.info.Hit", damage);

struct Packet {
    std::unique_ptr<std::int32_t> payload;
};
RTYPE_ECS_EVENT(Packet, "test.info.Packet");
}  // namespace rtype::test::event_info

namespace evi = rtype::test::event_info;

TEST(EventInfo, AcceptsAValidDescription) {
    EXPECT_TRUE(rtype::ecs::validate(rtype::ecs::EventTraits<evi::Hit>::makeInfo()));
}

TEST(EventInfo, AcceptsAMoveOnlyEvent) {
    EXPECT_TRUE(rtype::ecs::validate(rtype::ecs::EventTraits<evi::Packet>::makeInfo()));
}

TEST(EventInfo, RefusesAReplicatedEvent) {
    rtype::ecs::EventInfo info = rtype::ecs::EventTraits<evi::Hit>::makeInfo();
    info.type.replication = rtype::ecs::ReplicationMode::kEveryChange;
    const rtype::ecs::Result<void> valid = rtype::ecs::validate(info);
    ASSERT_FALSE(valid);
    EXPECT_EQ(valid.error().getKind(), rtype::ecs::ErrorKind::kInvalidType);
}

TEST(EventInfo, AppliesTheRulesOfEveryType) {
    const rtype::ecs::Result<void> valid =
        rtype::ecs::validate(rtype::ecs::EventInfo{.type = {.name = "", .size = 4, .alignment = 4}});
    ASSERT_FALSE(valid);
    EXPECT_EQ(valid.error().getKind(), rtype::ecs::ErrorKind::kInvalidType);
}
