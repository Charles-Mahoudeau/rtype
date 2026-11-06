/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** DynamicResourcesTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstring>
#include <span>
#include <string_view>

#include "rtype/ecs/exceptions/ResourceExceptions.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/system/DynamicResources.hpp"
#include "rtype/ecs/system/System.hpp"
#include "rtype/ecs/system/SystemAccess.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::dynamic_resources {
struct Score {
    int value;
};
RTYPE_ECS_RESOURCE(Score, "test.dynamicResources.Score", value);

struct Window {
    int width;
};
RTYPE_ECS_RESOURCE(Window, "test.dynamicResources.Window", width);
}  // namespace rtype::test::dynamic_resources

namespace {
using rtype::ecs::DynamicResources;
using rtype::ecs::ResourceId;
using rtype::ecs::ResourceList;
using rtype::ecs::System;
using rtype::ecs::World;
using rtype::test::dynamic_resources::Score;
using rtype::test::dynamic_resources::Window;

/// @return The int at the start of `bytes`.
int intOf(std::span<const std::byte> bytes) {
    int value = 0;
    std::memcpy(&value, bytes.data(), sizeof(int));
    return value;
}

/// @brief Adds 1 to resource 0 if it may be written, and copies its value into resource 1 (always written).
void bump(DynamicResources resources) {
    const std::span<std::byte> bytes = resources.getMut(0);
    if (!bytes.empty()) {
        const int value = intOf(bytes) + 1;
        std::memcpy(bytes.data(), &value, sizeof(int));
    }
}

class DynamicResourcesTest : public ::testing::Test {
  protected:
    void SetUp() override { world.getResources().insert(Score{.value = 10}); }

    World world;                                                   ///< The world under test.
    ResourceId score{world.getResources().registerType<Score>()};  ///< Score's id.
};
}  // namespace

TEST_F(DynamicResourcesTest, GivesTheBytesOfAWrittenResource) {
    System system{world, "test.bump", &bump, ResourceList{{{.id = score, .write = true}}}};
    system.run();
    EXPECT_EQ(world.getResources().require<Score>().value, 11);
}

TEST_F(DynamicResourcesTest, RefusesWritingAResourceDeclaredRead) {
    System system{world, "test.bump", &bump, ResourceList{{{.id = score, .write = false}}}};
    system.run();
    EXPECT_EQ(world.getResources().require<Score>().value, 10);
}

TEST_F(DynamicResourcesTest, DeclaresItsAccess) {
    const System reader{world, "test.reader", &bump, ResourceList{{{.id = score, .write = false}}}};
    const System writer{world, "test.writer", &bump, ResourceList{{{.id = score, .write = true}}}};
    ASSERT_EQ(reader.getAccess().getResources().size(), 1U);
    EXPECT_EQ(reader.getAccess().getResources().front().id, score);
    EXPECT_FALSE(reader.getAccess().getResources().front().write);
    EXPECT_TRUE(writer.getAccess().getResources().front().write);
    EXPECT_FALSE(writer.getAccess().isMainThread());
}

TEST_F(DynamicResourcesTest, ReportsAResourceWithoutValue) {
    const ResourceId window = world.getResources().registerType<Window>();
    System system{world, "test.bump", &bump, ResourceList{{{.id = window, .write = true}}}};
    EXPECT_NO_THROW(system.run());  // nothing to write: no bytes, no crash
}

TEST_F(DynamicResourcesTest, RefusesAnUnknownResource) {
    EXPECT_THROW((System{world, "test.bump", &bump, ResourceList{{{.id = 999, .write = false}}}}),
                 rtype::ecs::exceptions::ResourceRegistrationException);
}

TEST_F(DynamicResourcesTest, RefusesAMainThreadResource) {
    const ResourceId window = world.getResources().registerType<Window>(rtype::ecs::ResourceOptions{.nonSend = true});
    EXPECT_THROW((System{world, "test.bump", &bump, ResourceList{{{.id = window, .write = false}}}}),
                 rtype::ecs::exceptions::ResourceRegistrationException);
}
