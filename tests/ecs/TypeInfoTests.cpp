/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TypeInfoTests
*/

#include <gtest/gtest.h>

#include <string>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"

namespace {
/// @return A valid description of an 8-byte type with two fields.
rtype::ecs::TypeInfo makeScore() {
    return rtype::ecs::TypeInfo{.name = "game.Score",
                                .size = 8,
                                .alignment = 4,
                                .fields = {{.name = "value", .type = rtype::ecs::FieldType::kI32, .offset = 0},
                                           {.name = "combo", .type = rtype::ecs::FieldType::kI32, .offset = 4}}};
}
}  // namespace

TEST(TypeInfo, DefaultsAreCopyableNotReplicatedOnBothSides) {
    const rtype::ecs::TypeInfo info{};
    EXPECT_EQ(info.id, rtype::ecs::kInvalidTypeId);
    EXPECT_TRUE(info.copyable);
    EXPECT_EQ(info.presence, rtype::ecs::Presence::kBoth);
    EXPECT_EQ(info.replication, rtype::ecs::ReplicationMode::kNone);
}

TEST(TypeInfo, AcceptsAValidTypeOfAnyKind) {
    EXPECT_TRUE(rtype::ecs::validateType(makeScore(), "resource"));
    rtype::ecs::TypeInfo moveOnly = makeScore();
    moveOnly.copyable = false;  // only components require copyability
    EXPECT_TRUE(rtype::ecs::validateType(moveOnly, "resource"));
}

TEST(TypeInfo, MessagesNameTheKind) {
    rtype::ecs::TypeInfo info = makeScore();
    info.alignment = 3;
    const rtype::ecs::Result<void> result = rtype::ecs::validateType(info, "resource");
    ASSERT_FALSE(result);
    const std::string message{result.error().getMessage()};
    EXPECT_NE(message.find("Resource 'game.Score'"), std::string::npos) << message;
}

TEST(TypeInfo, MessagesNameTheKindWhenTheNameIsMissing) {
    const rtype::ecs::Result<void> result = rtype::ecs::validateType(rtype::ecs::TypeInfo{}, "resource");
    ASSERT_FALSE(result);
    EXPECT_NE(std::string{result.error().getMessage()}.find("A resource needs a name"), std::string::npos);
}
