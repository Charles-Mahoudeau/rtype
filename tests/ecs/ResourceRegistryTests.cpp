/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ResourceRegistryTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <utility>

#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/exceptions/ResourceExceptions.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceRegistry.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/type/NameHash.hpp"

namespace rtype::test::resource_registry {
struct Time {
    float delta;
    float fixedDelta;
};
RTYPE_ECS_RESOURCE(Time, "test.registry.Time");

struct Backend {
    std::unique_ptr<std::int32_t> window;
};
RTYPE_ECS_RESOURCE(Backend, "test.registry.Backend");

/// @brief Another layout under Time's name.
struct OtherTime {
    double delta;
};
RTYPE_ECS_RESOURCE(OtherTime, "test.registry.Time");
}  // namespace rtype::test::resource_registry

namespace reg = rtype::test::resource_registry;

namespace {
/// @return A run-time description, as a Luau script would provide it.
rtype::ecs::ResourceInfo makeScore() {
    return rtype::ecs::ResourceInfo{.type = {.name = "test.registry.Score", .size = 4, .alignment = 4}};
}
}  // namespace

TEST(ResourceRegistry, GivesDenseIdsFromZero) {
    rtype::ecs::ResourceRegistry registry;
    EXPECT_EQ(registry.registerType<reg::Time>(), 0U);
    EXPECT_EQ(registry.registerType<reg::Backend>(), 1U);
    EXPECT_EQ(registry.getCount(), 2U);
}

TEST(ResourceRegistry, RegisteringTheSameResourceTwiceReturnsTheSameId) {
    rtype::ecs::ResourceRegistry registry;
    const rtype::ecs::ResourceId id = registry.registerType<reg::Time>();
    EXPECT_EQ(registry.registerType<reg::Time>(), id);
    EXPECT_EQ(registry.getCount(), 1U);
}

TEST(ResourceRegistry, FindsResourcesByIdNameHashAndType) {
    rtype::ecs::ResourceRegistry registry;
    const rtype::ecs::ResourceId id = registry.registerType<reg::Time>();
    const rtype::ecs::ResourceInfo* info = registry.findById(id);
    ASSERT_NE(info, nullptr);
    EXPECT_EQ(info->type.id, id);
    EXPECT_EQ(info->type.name, "test.registry.Time");
    EXPECT_EQ(registry.findByName("test.registry.Time"), info);
    EXPECT_EQ(registry.findByHash(rtype::ecs::hashName("test.registry.Time")), info);
    EXPECT_EQ(registry.find<reg::Time>(), info);
}

TEST(ResourceRegistry, ReturnsNullForUnknownResources) {
    const rtype::ecs::ResourceRegistry registry;
    EXPECT_EQ(registry.findById(0), nullptr);
    EXPECT_EQ(registry.findByName("test.registry.Time"), nullptr);
    EXPECT_EQ(registry.findByHash(rtype::ecs::hashName("test.registry.Time")), nullptr);
    EXPECT_EQ(registry.find<reg::Time>(), nullptr);
}

TEST(ResourceRegistry, AppliesTheMainThreadOption) {
    rtype::ecs::ResourceRegistry registry;
    registry.registerType<reg::Backend>({.nonSend = true});
    registry.registerType<reg::Time>();
    EXPECT_TRUE(registry.find<reg::Backend>()->nonSend);
    EXPECT_FALSE(registry.find<reg::Time>()->nonSend);
}

TEST(ResourceRegistry, RegistersRunTimeDescriptions) {
    rtype::ecs::ResourceRegistry registry;
    const rtype::ecs::Result<rtype::ecs::ResourceId> id = registry.registerType(makeScore());
    ASSERT_TRUE(id);
    EXPECT_EQ(registry.findByName("test.registry.Score")->type.id, *id);
    const rtype::ecs::Result<rtype::ecs::ResourceId> again = registry.registerType(makeScore());
    ASSERT_TRUE(again);
    EXPECT_EQ(*again, *id);
}

TEST(ResourceRegistry, FindAcceptsARunTimeDescriptionOfTheSameLayout) {
    rtype::ecs::ResourceRegistry registry;
    rtype::ecs::ResourceInfo info{.type = {.name = "test.registry.Time", .size = sizeof(reg::Time), .alignment = 4}};
    ASSERT_TRUE(registry.registerType(std::move(info)));
    EXPECT_NE(registry.find<reg::Time>(), nullptr);
}

TEST(ResourceRegistry, FindReturnsNullForARunTimeResourceThatOnlySharesTheName) {
    rtype::ecs::ResourceRegistry registry;
    ASSERT_TRUE(registry.registerType(rtype::ecs::ResourceInfo{.type = {.name = "test.registry.Time", .size = 4}}));
    EXPECT_EQ(registry.find<reg::Time>(), nullptr);
}

TEST(ResourceRegistry, RefusesInvalidDescriptions) {
    rtype::ecs::ResourceRegistry registry;
    const auto refused = [&registry](rtype::ecs::ResourceInfo info) {
        const rtype::ecs::Result<rtype::ecs::ResourceId> id = registry.registerType(std::move(info));
        return !id && id.error().getKind() == rtype::ecs::ErrorKind::kInvalidType;
    };
    EXPECT_TRUE(refused(rtype::ecs::ResourceInfo{.type = {.name = "", .size = 4, .alignment = 4}}));
    EXPECT_TRUE(refused(rtype::ecs::ResourceInfo{.type = {.name = "test.registry.A", .size = 4, .alignment = 3}}));
    EXPECT_TRUE(refused(rtype::ecs::ResourceInfo{.type = {.name = "test.registry.B", .size = 4, .alignment = 0}}));
    EXPECT_TRUE(refused(rtype::ecs::ResourceInfo{.type = {.name = "test.registry.C", .size = 6, .alignment = 4}}));
    rtype::ecs::ResourceInfo noCopy = rtype::ecs::ResourceTraits<reg::Backend>::makeInfo();
    noCopy.type.name = "test.registry.D";
    noCopy.type.copyable = true;
    EXPECT_TRUE(refused(std::move(noCopy)));
    EXPECT_EQ(registry.getCount(), 0U);
}

TEST(ResourceRegistry, RefusesTheSameNameWithAnotherLayout) {
    rtype::ecs::ResourceRegistry registry;
    registry.registerType<reg::Time>();
    const rtype::ecs::Result<rtype::ecs::ResourceId> id =
        registry.registerType(rtype::ecs::ResourceInfo{.type = {.name = "test.registry.Time", .size = 4}});
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kLayoutMismatch);
}

TEST(ResourceRegistry, RefusesTheOtherMainThreadFlag) {
    rtype::ecs::ResourceRegistry registry;
    rtype::ecs::ResourceInfo info = makeScore();
    ASSERT_TRUE(registry.registerType(info));
    info.nonSend = true;
    const rtype::ecs::Result<rtype::ecs::ResourceId> id = registry.registerType(std::move(info));
    ASSERT_FALSE(id);
    EXPECT_EQ(id.error().getKind(), rtype::ecs::ErrorKind::kOptionsMismatch);
}

TEST(ResourceRegistry, TypedRegistrationThrowsOnConflicts) {
    rtype::ecs::ResourceRegistry registry;
    registry.registerType<reg::Time>();
    EXPECT_THROW(registry.registerType<reg::OtherTime>(), rtype::ecs::exceptions::ResourceRegistrationException);
    EXPECT_THROW(registry.registerType<reg::Time>({.nonSend = true}),
                 rtype::ecs::exceptions::ResourceRegistrationException);
}

TEST(ResourceRegistry, PointersStayValidAfterManyRegistrations) {
    rtype::ecs::ResourceRegistry registry;
    registry.registerType<reg::Time>();
    const rtype::ecs::ResourceInfo* time = registry.find<reg::Time>();
    for (int index = 0; index < 100; ++index) {
        ASSERT_TRUE(registry.registerType(rtype::ecs::ResourceInfo{
            .type = {.name = std::format("test.registry.Many{}", index), .size = 4, .alignment = 4}}));
    }
    EXPECT_EQ(registry.find<reg::Time>(), time);
}

TEST(ResourceRegistry, MessagesNameTheResource) {
    rtype::ecs::ResourceRegistry registry;
    rtype::ecs::ResourceInfo info = makeScore();
    ASSERT_TRUE(registry.registerType(info));
    info.nonSend = true;
    const rtype::ecs::Result<rtype::ecs::ResourceId> id = registry.registerType(std::move(info));
    ASSERT_FALSE(id);
    EXPECT_EQ(std::string{id.error().getMessage()},
              "Resource 'test.registry.Score' is already registered with thread 'any', not 'main'");
}

TEST(ResourceRegistry, KeepsItsResourcesWhenMoved) {
    rtype::ecs::ResourceRegistry registry;
    registry.registerType<reg::Time>();
    const rtype::ecs::ResourceRegistry moved{std::move(registry)};
    EXPECT_NE(moved.find<reg::Time>(), nullptr);
    EXPECT_EQ(moved.getCount(), 1U);
}
