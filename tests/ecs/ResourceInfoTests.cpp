/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceInfoTests
*/

#include <gtest/gtest.h>

#include <string>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/type/Field.hpp"

namespace {
/// @return A valid run-time description, as a Luau script would declare a resource.
rtype::ecs::ResourceInfo makeScore() {
    return rtype::ecs::ResourceInfo{
        .type = {.name = "game.Score",
                 .size = 4,
                 .alignment = 4,
                 .replication = rtype::ecs::ReplicationMode::kEveryChange,
                 .fields = {{.name = "value", .type = rtype::ecs::FieldType::kI32, .offset = 0}}}};
}
}  // namespace

TEST(ResourceInfo, DefaultsAreUsableFromAnyThread) {
    const rtype::ecs::ResourceInfo info{};
    EXPECT_EQ(info.type.id, rtype::ecs::kInvalidResourceId);
    EXPECT_FALSE(info.nonSend);
}

TEST(ResourceInfo, AcceptsAReplicatedResourceWithFields) { EXPECT_TRUE(rtype::ecs::validate(makeScore())); }

TEST(ResourceInfo, AcceptsAMoveOnlyResource) {
    rtype::ecs::ResourceInfo backend{.type = {.name = "rtype.Backend", .size = 8, .alignment = 8, .copyable = false},
                                     .nonSend = true};
    EXPECT_TRUE(rtype::ecs::validate(backend));
}

TEST(ResourceInfo, RejectsAnInvalidDescriptionNamingTheResource) {
    rtype::ecs::ResourceInfo info = makeScore();
    info.type.fields.clear();  // replicated, but nothing to serialize
    const rtype::ecs::Result<void> result = rtype::ecs::validate(info);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().getKind(), rtype::ecs::ErrorKind::kInvalidType);
    const std::string message{result.error().getMessage()};
    EXPECT_NE(message.find("Resource 'game.Score'"), std::string::npos) << message;
    EXPECT_NE(message.find("a replicated resource needs fields"), std::string::npos) << message;
}
