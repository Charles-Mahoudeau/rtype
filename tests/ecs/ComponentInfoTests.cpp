/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentInfoTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <initializer_list>
#include <string>
#include <string_view>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/error/Error.hpp"

namespace {
/// @return A valid description of a `{ std::int32_t value; std::int32_t max; }` component.
rtype::ecs::ComponentInfo makeHealth() {
    return rtype::ecs::ComponentInfo{
        .name = "rtype.Health",
        .size = 8,
        .alignment = 4,
        .fields = {{.name = "value", .type = rtype::ecs::FieldType::kI32, .offset = 0},
                   {.name = "max", .type = rtype::ecs::FieldType::kI32, .offset = 4}},
    };
}

/// @brief Expects validate() to reject the description with a message containing every given word.
void expectInvalid(const rtype::ecs::ComponentInfo& info, std::initializer_list<std::string_view> words) {
    const rtype::ecs::Result<void> result = rtype::ecs::validate(info);
    ASSERT_FALSE(result) << "expected '" << info.name << "' to be rejected";
    EXPECT_EQ(result.error().getKind(), rtype::ecs::ErrorKind::kInvalidComponent);
    const std::string message{result.error().getMessage()};
    for (const std::string_view word : words) {
        EXPECT_NE(message.find(word), std::string::npos) << "'" << word << "' missing from: " << message;
    }
}
}  // namespace

TEST(FieldLayout, MatchesTheCppTypes) {
    using rtype::ecs::FieldType;
    using rtype::ecs::getFieldLayout;
    EXPECT_EQ(getFieldLayout(FieldType::kBool).size, sizeof(bool));
    EXPECT_EQ(getFieldLayout(FieldType::kI8).size, 1U);
    EXPECT_EQ(getFieldLayout(FieldType::kU16).size, 2U);
    EXPECT_EQ(getFieldLayout(FieldType::kI32).size, 4U);
    EXPECT_EQ(getFieldLayout(FieldType::kF32).size, sizeof(float));
    EXPECT_EQ(getFieldLayout(FieldType::kF64).size, sizeof(double));
    EXPECT_EQ(getFieldLayout(FieldType::kU64).alignment, alignof(std::uint64_t));
    EXPECT_EQ(getFieldLayout(FieldType::kVec2).size, 8U);
    EXPECT_EQ(getFieldLayout(FieldType::kVec3).size, 12U);
    EXPECT_EQ(getFieldLayout(FieldType::kVec4).size, 16U);
    EXPECT_EQ(getFieldLayout(FieldType::kVec4).alignment, 4U);
    EXPECT_EQ(getFieldLayout(FieldType::kEntity).size, 8U);
    EXPECT_EQ(getFieldLayout(FieldType::kAssetId).size, 8U);
}

TEST(ComponentInfo, DefaultsAreNotReplicatedSparseSetOnBothSides) {
    const rtype::ecs::ComponentInfo info{};
    EXPECT_EQ(info.id, rtype::ecs::kInvalidComponentId);
    EXPECT_EQ(info.storage, rtype::ecs::StorageKind::kSparseSet);
    EXPECT_EQ(info.presence, rtype::ecs::Presence::kBoth);
    EXPECT_EQ(info.replication, rtype::ecs::ReplicationMode::kNone);
    EXPECT_EQ(info.hooks.construct, nullptr);
}

TEST(Validate, AcceptsAValidComponent) { EXPECT_TRUE(rtype::ecs::validate(makeHealth())); }

TEST(Validate, AcceptsATag) {
    EXPECT_TRUE(rtype::ecs::validate(rtype::ecs::ComponentInfo{.name = "rtype.Dead", .size = 0, .alignment = 1}));
}

TEST(Validate, AcceptsAReplicatedComponentAndAReplicatedTag) {
    rtype::ecs::ComponentInfo health = makeHealth();
    health.replication = rtype::ecs::ReplicationMode::kEveryChange;
    EXPECT_TRUE(rtype::ecs::validate(health));

    const rtype::ecs::ComponentInfo tag{
        .name = "rtype.Player", .size = 0, .alignment = 1, .replication = rtype::ecs::ReplicationMode::kSpawnOnly};
    EXPECT_TRUE(rtype::ecs::validate(tag));
}

TEST(Validate, AcceptsAFixedArrayField) {
    const rtype::ecs::ComponentInfo info{
        .name = "game.Path",
        .size = 32,
        .alignment = 4,
        .fields = {{.name = "points", .type = rtype::ecs::FieldType::kVec2, .offset = 0, .count = 4}},
    };
    EXPECT_TRUE(rtype::ecs::validate(info));
}

TEST(Validate, RejectsAnEmptyName) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.name.clear();
    expectInvalid(info, {"name"});
}

TEST(Validate, RejectsAnAlignmentThatIsNotAPowerOfTwo) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.alignment = 0;
    expectInvalid(info, {"rtype.Health", "alignment 0"});
    info.alignment = 3;
    expectInvalid(info, {"rtype.Health", "alignment 3"});
}

TEST(Validate, RejectsASizeThatIsNotAMultipleOfTheAlignment) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.size = 6;
    expectInvalid(info, {"rtype.Health", "size 6"});
}

TEST(Validate, RejectsATagWithFields) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.size = 0;
    expectInvalid(info, {"rtype.Health", "tag"});
}

TEST(Validate, RejectsAReplicatedClientOnlyComponent) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.presence = rtype::ecs::Presence::kClientOnly;
    info.replication = rtype::ecs::ReplicationMode::kEveryChange;
    expectInvalid(info, {"rtype.Health", "client-only"});
}

TEST(Validate, RejectsAReplicatedComponentWithoutFields) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.fields.clear();
    info.replication = rtype::ecs::ReplicationMode::kEveryChange;
    expectInvalid(info, {"rtype.Health", "fields"});
}

TEST(Validate, RejectsAFieldWithoutName) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.fields.at(1).name.clear();
    expectInvalid(info, {"rtype.Health", "offset 4", "no name"});
}

TEST(Validate, RejectsADuplicateField) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.fields.at(1).name = "value";
    expectInvalid(info, {"rtype.Health", "'value'", "twice"});
}

TEST(Validate, RejectsAnEmptyArrayField) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.fields.at(1).count = 0;
    expectInvalid(info, {"rtype.Health", "'max'", "count of 0"});
}

TEST(Validate, RejectsAMisalignedField) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.fields.at(1).offset = 2;
    expectInvalid(info, {"rtype.Health", "'max'", "aligned"});
}

TEST(Validate, RejectsAFieldPastTheEnd) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.fields.at(1).offset = 8;
    expectInvalid(info, {"rtype.Health", "'max'", "ends at byte 12"});
}

TEST(Validate, RejectsAHugeArrayWithoutOverflowing) {
    rtype::ecs::ComponentInfo info = makeHealth();
    info.fields.at(1).count = 0xFFFFFFFFU;
    expectInvalid(info, {"rtype.Health", "'max'", "past the component size"});
}

TEST(Validate, RejectsOverlappingFields) {
    rtype::ecs::ComponentInfo info{
        .name = "game.Shield",
        .size = 16,
        .alignment = 4,
        .fields = {{.name = "strength", .type = rtype::ecs::FieldType::kF32, .offset = 4},
                   {.name = "direction", .type = rtype::ecs::FieldType::kVec2, .offset = 0}},
    };
    expectInvalid(info, {"game.Shield", "'direction'", "'strength'", "overlap"});
}
