/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentRegistryTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <string>
#include <utility>
#include <vector>

#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentRegistry.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/exceptions/ComponentExceptions.hpp"
#include "rtype/ecs/type/NameHash.hpp"

namespace rtype::test::registry {
struct Health {
    std::int32_t value;
    std::int32_t max;
};
RTYPE_ECS_COMPONENT(Health, "rtype.Health", value, max);

struct Transform {
    float x;
    float y;
};
RTYPE_ECS_COMPONENT(Transform, "rtype.Transform", x, y);

struct Dead {};
RTYPE_ECS_COMPONENT(Dead, "rtype.Dead");

/// @brief Another struct claiming the name "rtype.Health", with another layout.
struct OtherHealth {
    float value;
};
RTYPE_ECS_COMPONENT(OtherHealth, "rtype.Health", value);
}  // namespace rtype::test::registry

namespace {
/// @return A run-time description, as a script would provide it.
rtype::ecs::ComponentInfo makeShield() {
    return rtype::ecs::ComponentInfo{
        .name = "game.Shield",
        .size = 8,
        .alignment = 4,
        .fields = {{.name = "strength", .type = rtype::ecs::FieldType::kF32, .offset = 0},
                   {.name = "regen", .type = rtype::ecs::FieldType::kF32, .offset = 4}},
    };
}
}  // namespace

TEST(ComponentRegistry, GivesDenseIdsFromZero) {
    rtype::ecs::ComponentRegistry registry;
    EXPECT_EQ(registry.registerComponent<rtype::test::registry::Health>(), 0U);
    EXPECT_EQ(registry.registerComponent<rtype::test::registry::Transform>(), 1U);
    EXPECT_EQ(registry.registerComponent<rtype::test::registry::Dead>(), 2U);
    EXPECT_EQ(registry.getCount(), 3U);
}

TEST(ComponentRegistry, RegisteringTheSameComponentTwiceReturnsTheSameId) {
    rtype::ecs::ComponentRegistry registry;
    const rtype::ecs::ComponentId first = registry.registerComponent<rtype::test::registry::Health>();
    EXPECT_EQ(registry.registerComponent<rtype::test::registry::Health>(), first);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(ComponentRegistry, FindsComponentsByIdNameHashAndType) {
    rtype::ecs::ComponentRegistry registry;
    const rtype::ecs::ComponentId id = registry.registerComponent<rtype::test::registry::Health>();

    const rtype::ecs::ComponentInfo* byId = registry.findById(id);
    ASSERT_NE(byId, nullptr);
    EXPECT_EQ(byId->name, "rtype.Health");
    EXPECT_EQ(byId->id, id);
    EXPECT_EQ(registry.findByName("rtype.Health"), byId);
    EXPECT_EQ(registry.findByHash(rtype::ecs::hashName("rtype.Health")), byId);
    EXPECT_EQ(registry.find<rtype::test::registry::Health>(), byId);
}

TEST(ComponentRegistry, FindReturnsNullForAnotherLayoutUnderTheSameName) {
    rtype::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerComponent<rtype::test::registry::Health>());
    EXPECT_EQ(registry.find<rtype::test::registry::OtherHealth>(), nullptr);  // "rtype.Health", but not this layout
    EXPECT_NE(registry.find<rtype::test::registry::Health>(), nullptr);
}

TEST(ComponentRegistry, FindReturnsNullForARunTimeComponentThatOnlySharesTheName) {
    rtype::ecs::ComponentRegistry registry;
    rtype::ecs::ComponentInfo script = makeShield();  // 8 bytes like Health, other fields
    script.name = "rtype.Health";
    ASSERT_TRUE(registry.registerComponent(std::move(script)));
    EXPECT_EQ(registry.find<rtype::test::registry::Health>(), nullptr);
    EXPECT_THROW(static_cast<void>(registry.registerComponent<rtype::test::registry::Health>()),
                 rtype::ecs::exceptions::ComponentRegistrationException);
}

TEST(ComponentRegistry, FindAcceptsARunTimeDescriptionOfTheSameLayout) {
    rtype::ecs::ComponentRegistry registry;
    // A script describing exactly the C++ layout is the same component.
    ASSERT_TRUE(registry.registerComponent(rtype::ecs::ComponentTraits<rtype::test::registry::Health>::makeInfo()));
    EXPECT_NE(registry.find<rtype::test::registry::Health>(), nullptr);
}

TEST(ComponentRegistry, ReturnsNullForUnknownComponents) {
    rtype::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerComponent<rtype::test::registry::Health>());
    EXPECT_EQ(registry.findById(1), nullptr);
    EXPECT_EQ(registry.findById(rtype::ecs::kInvalidComponentId), nullptr);
    EXPECT_EQ(registry.findByName("rtype.Unknown"), nullptr);
    EXPECT_EQ(registry.findByHash(rtype::ecs::hashName("rtype.Unknown")), nullptr);
    EXPECT_EQ(registry.find<rtype::test::registry::Transform>(), nullptr);
}

TEST(ComponentRegistry, SetsTheLayoutHash) {
    rtype::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerComponent<rtype::test::registry::Health>());
    const rtype::ecs::ComponentInfo* info = registry.find<rtype::test::registry::Health>();
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->layoutHash, rtype::ecs::computeLayoutHash(*info));
    EXPECT_NE(info->layoutHash, 0U);
}

TEST(ComponentRegistry, AppliesTheOptions) {
    rtype::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerComponent<rtype::test::registry::Transform>(
        {.storage = rtype::ecs::StorageKind::kTable, .replication = rtype::ecs::ReplicationMode::kEveryChange}));
    const rtype::ecs::ComponentInfo* info = registry.find<rtype::test::registry::Transform>();
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->storage, rtype::ecs::StorageKind::kTable);
    EXPECT_EQ(info->presence, rtype::ecs::Presence::kBoth);
    EXPECT_EQ(info->replication, rtype::ecs::ReplicationMode::kEveryChange);
}

TEST(ComponentRegistry, ListsOnlyReplicatedComponents) {
    rtype::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerComponent<rtype::test::registry::Health>());
    const rtype::ecs::ComponentId transform = registry.registerComponent<rtype::test::registry::Transform>(
        {.replication = rtype::ecs::ReplicationMode::kEveryChange});
    const rtype::ecs::ComponentId dead = registry.registerComponent<rtype::test::registry::Dead>(
        {.replication = rtype::ecs::ReplicationMode::kSpawnOnly});
    static_cast<void>(registry.registerComponent<rtype::test::registry::Transform>(
        {.replication = rtype::ecs::ReplicationMode::kEveryChange}));  // again: listed once

    const std::vector<rtype::ecs::ComponentId> replicated{registry.getReplicatedIds().begin(),
                                                          registry.getReplicatedIds().end()};
    EXPECT_EQ(replicated, (std::vector<rtype::ecs::ComponentId>{transform, dead}));
}

TEST(ComponentRegistry, RegistersRunTimeDescriptions) {
    rtype::ecs::ComponentRegistry registry;
    const rtype::ecs::Result<rtype::ecs::ComponentId> id = registry.registerComponent(makeShield());
    ASSERT_TRUE(id);
    EXPECT_EQ(*id, 0U);
    const rtype::ecs::Result<rtype::ecs::ComponentId> again = registry.registerComponent(makeShield());
    ASSERT_TRUE(again);
    EXPECT_EQ(*again, *id);
    EXPECT_NE(registry.findByName("game.Shield"), nullptr);
}

TEST(ComponentRegistry, RefusesAnInvalidDescription) {
    rtype::ecs::ComponentRegistry registry;
    rtype::ecs::ComponentInfo info = makeShield();
    info.alignment = 3;
    const rtype::ecs::Result<rtype::ecs::ComponentId> id = registry.registerComponent(std::move(info));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kInvalidComponent);
    EXPECT_EQ(registry.getCount(), 0U);
}

TEST(ComponentRegistry, RefusesTheSameNameWithAnotherLayout) {
    rtype::ecs::ComponentRegistry registry;
    ASSERT_TRUE(registry.registerComponent(makeShield()));
    rtype::ecs::ComponentInfo bigger = makeShield();
    bigger.size = 16;
    const rtype::ecs::Result<rtype::ecs::ComponentId> id = registry.registerComponent(std::move(bigger));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kLayoutMismatch);
    EXPECT_NE(std::string{id.error().getMessage()}.find("game.Shield"), std::string::npos);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(ComponentRegistry, RefusesTheSameLayoutWithOtherHooks) {
    rtype::ecs::ComponentRegistry registry;
    ASSERT_TRUE(registry.registerComponent(makeShield()));
    rtype::ecs::ComponentInfo managed = makeShield();  // same name, size and fields, but needs destroying
    managed.hooks.destruct = [](void* /*target*/) noexcept {};
    const rtype::ecs::Result<rtype::ecs::ComponentId> id = registry.registerComponent(std::move(managed));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kLayoutMismatch);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(ComponentRegistry, RefusesTheSameNameWithOtherOptions) {
    rtype::ecs::ComponentRegistry registry;
    ASSERT_TRUE(registry.registerComponent(makeShield()));
    rtype::ecs::ComponentInfo replicated = makeShield();
    replicated.replication = rtype::ecs::ReplicationMode::kEveryChange;
    const rtype::ecs::Result<rtype::ecs::ComponentId> id = registry.registerComponent(std::move(replicated));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kOptionsMismatch);
    const std::string message{id.error().getMessage()};
    EXPECT_NE(message.find("replication 'none', not 'everyChange'"), std::string::npos) << message;
    EXPECT_TRUE(registry.getReplicatedIds().empty());
}

TEST(ComponentRegistry, TypedRegistrationThrowsOnConflicts) {
    rtype::ecs::ComponentRegistry registry;
    static_cast<void>(registry.registerComponent<rtype::test::registry::Health>());
    EXPECT_THROW(static_cast<void>(registry.registerComponent<rtype::test::registry::OtherHealth>()),
                 rtype::ecs::exceptions::ComponentRegistrationException);
    EXPECT_THROW(static_cast<void>(registry.registerComponent<rtype::test::registry::Health>(
                     {.replication = rtype::ecs::ReplicationMode::kEveryChange})),
                 rtype::ecs::exceptions::ComponentRegistrationException);
}

TEST(ComponentRegistry, PointersStayValidAfterManyRegistrations) {
    rtype::ecs::ComponentRegistry registry;
    const rtype::ecs::ComponentId health = registry.registerComponent<rtype::test::registry::Health>();
    const rtype::ecs::ComponentInfo* info = registry.findById(health);
    for (int i = 0; i < 500; ++i) {
        rtype::ecs::ComponentInfo other = makeShield();
        other.name = std::format("game.Shield{}", i);
        ASSERT_TRUE(registry.registerComponent(std::move(other)));
    }
    EXPECT_EQ(registry.findById(health), info);
    EXPECT_EQ(info->name, "rtype.Health");
}

TEST(ComponentRegistry, KeepsItsComponentsWhenMoved) {
    rtype::ecs::ComponentRegistry registry;
    const rtype::ecs::ComponentId health = registry.registerComponent<rtype::test::registry::Health>();
    const rtype::ecs::ComponentRegistry moved{std::move(registry)};
    ASSERT_NE(moved.findById(health), nullptr);
    EXPECT_EQ(moved.findByName("rtype.Health")->id, health);
}
