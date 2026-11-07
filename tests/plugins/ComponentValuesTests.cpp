/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ComponentValuesTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "plugins/capsule/ComponentValues.hpp"
#include "plugins/capsule/exceptions/CapsuleException.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/hierarchy/Children.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/type/TypeInfo.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
/// @brief Number of Counter instances alive, to detect leaks and double destruction.
int liveTracked = 0;

/// @brief Counts its own instances.
struct Counter {
    Counter() noexcept { ++liveTracked; }
    ~Counter() { --liveTracked; }
    Counter(const Counter& /*other*/) noexcept { ++liveTracked; }
    Counter& operator=(const Counter& /*other*/) noexcept = default;
    Counter(Counter&& /*other*/) noexcept { ++liveTracked; }
    Counter& operator=(Counter&& /*other*/) noexcept = default;
};
}  // namespace

namespace rtype::test::capsule_values {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.capsulevalues.Health", value);

struct Enemy {};
RTYPE_ECS_COMPONENT(Enemy, "test.capsulevalues.Enemy");

/// @brief A component that owns memory and counts its instances.
struct Tracked {
    std::vector<std::int32_t> path;
    Counter counter;
};
RTYPE_ECS_COMPONENT(Tracked, "test.capsulevalues.Tracked");
}  // namespace rtype::test::capsule_values

namespace cv = rtype::test::capsule_values;

namespace {
/// @return The bytes of a value, as the Luau bridge passes them.
template <typename T>
std::span<const std::byte> bytesOf(const T& value) {
    return std::as_bytes(std::span{&value, 1});
}
}  // namespace

TEST(ComponentValues, StartsEmpty) {
    const rtype::ecs::World world;
    const rtype::plugins::capsule::ComponentValues values;
    EXPECT_EQ(values.getCount(), 0U);
    EXPECT_FALSE(values.has(0));
    EXPECT_TRUE(values.get(0).empty());
    EXPECT_EQ(values.get<cv::Health>(world.getComponents()), nullptr);
}

TEST(ComponentValues, SetsACppValueAndRegistersItsType) {
    rtype::ecs::World world;
    rtype::plugins::capsule::ComponentValues values;
    values.set(world.getComponents(), cv::Health{.value = 3});
    const auto id = world.getComponents().find("test.capsulevalues.Health");
    ASSERT_TRUE(id);
    EXPECT_TRUE(values.has(*id));
    ASSERT_NE(values.get<cv::Health>(world.getComponents()), nullptr);
    EXPECT_EQ(values.get<cv::Health>(world.getComponents())->value, 3);
    EXPECT_EQ(values.getCount(), 1U);
    EXPECT_EQ(values.getId(0), *id);
}

TEST(ComponentValues, SetsAValueByIdAndBytes) {
    rtype::ecs::World world;
    const rtype::ecs::ComponentId id = world.getComponents().registerType<cv::Health>();
    rtype::plugins::capsule::ComponentValues values;
    values.set(world.getComponents(), id, bytesOf(cv::Health{.value = 7}));
    EXPECT_EQ(values.get<cv::Health>(world.getComponents())->value, 7);
    EXPECT_EQ(values.get(id).size(), sizeof(cv::Health));
}

TEST(ComponentValues, ReplacesAValueSetAgainInPlace) {
    rtype::ecs::World world;
    rtype::plugins::capsule::ComponentValues values;
    values.set(world.getComponents(), cv::Health{.value = 1});
    values.set(world.getComponents(), cv::Enemy{});
    values.set(world.getComponents(), cv::Health{.value = 2});
    EXPECT_EQ(values.getCount(), 2U);
    EXPECT_EQ(values.get<cv::Health>(world.getComponents())->value, 2);
    EXPECT_EQ(values.getId(0), *world.getComponents().find("test.capsulevalues.Health"));  // keeps its first place
}

TEST(ComponentValues, HoldsTags) {
    rtype::ecs::World world;
    rtype::plugins::capsule::ComponentValues values;
    values.set(world.getComponents(), cv::Enemy{});
    const rtype::ecs::ComponentId id = *world.getComponents().find("test.capsulevalues.Enemy");
    EXPECT_TRUE(values.has(id));
    EXPECT_TRUE(values.get(id).empty());
    EXPECT_TRUE(values.getValue(0).empty());
}

TEST(ComponentValues, CopiesAndDestroysValuesThatOwnMemory) {
    liveTracked = 0;
    {
        rtype::ecs::World world;
        rtype::plugins::capsule::ComponentValues values;
        values.set(world.getComponents(), cv::Tracked{.path = {1, 2, 3}, .counter = {}});
        EXPECT_EQ(liveTracked, 1);
        const cv::Tracked source{.path = {4, 5}, .counter = {}};
        values.set(world.getComponents(), *world.getComponents().find("test.capsulevalues.Tracked"), bytesOf(source));
        EXPECT_EQ(liveTracked, 2);  // the source and the copy; the replaced value is gone
        EXPECT_EQ(values.get<cv::Tracked>(world.getComponents())->path, (std::vector<std::int32_t>{4, 5}));
        rtype::plugins::capsule::ComponentValues moved{std::move(values)};
        EXPECT_EQ(liveTracked, 2);
        EXPECT_TRUE(moved.remove(*world.getComponents().find("test.capsulevalues.Tracked")));
        EXPECT_EQ(liveTracked, 1);
    }
    EXPECT_EQ(liveTracked, 0);
}

TEST(ComponentValues, RemovesAValue) {
    rtype::ecs::World world;
    rtype::plugins::capsule::ComponentValues values;
    values.set(world.getComponents(), cv::Health{.value = 1});
    const rtype::ecs::ComponentId id = *world.getComponents().find("test.capsulevalues.Health");
    EXPECT_TRUE(values.remove(id));
    EXPECT_FALSE(values.has(id));
    EXPECT_FALSE(values.remove(id));
}

TEST(ComponentValues, RefusesAnUnknownIdOrBytesOfTheWrongSize) {
    rtype::ecs::World world;
    const rtype::ecs::ComponentId id = world.getComponents().registerType<cv::Health>();
    rtype::plugins::capsule::ComponentValues values;
    const std::int64_t tooBig = 0;
    EXPECT_THROW(values.set(world.getComponents(), 99, bytesOf(cv::Health{})),
                 rtype::plugins::capsule::exceptions::CapsuleException);
    try {
        values.set(world.getComponents(), id, bytesOf(tooBig));
        FAIL() << "no CapsuleException";
    } catch (const rtype::plugins::capsule::exceptions::CapsuleException& error) {
        EXPECT_EQ(std::string{error.what()}, "Component 'test.capsulevalues.Health' takes 4 bytes, not 8");
    }
    EXPECT_EQ(values.getCount(), 0U);
}

TEST(ComponentValues, RefusesTheHierarchyComponents) {
    rtype::ecs::World world;
    const rtype::ecs::ComponentId parent =
        world.getComponents().registerType<rtype::ecs::Parent>(rtype::ecs::kParentOptions);
    const rtype::ecs::ComponentId children = world.getComponents().registerType<rtype::ecs::Children>();
    rtype::plugins::capsule::ComponentValues values;
    try {
        values.set(world.getComponents(), parent, bytesOf(rtype::ecs::Parent{}));
        FAIL() << "no CapsuleException";
    } catch (const rtype::plugins::capsule::exceptions::CapsuleException& error) {
        EXPECT_EQ(std::string{error.what()},
                  "Component 'rtype.Parent' cannot be a capsule value: a capsule's nodes set the hierarchy");
    }
    const rtype::ecs::Children list{};
    EXPECT_THROW(values.set(world.getComponents(), children, bytesOf(list)),
                 rtype::plugins::capsule::exceptions::CapsuleException);
    EXPECT_EQ(values.getCount(), 0U);
}

TEST(ComponentValues, KeepsThePresenceOfEachValue) {
    rtype::ecs::World world;
    static_cast<void>(world.getComponents().registerType<cv::Health>({.presence = rtype::ecs::Presence::kServerOnly}));
    rtype::plugins::capsule::ComponentValues values;
    values.set(world.getComponents(), cv::Health{.value = 1});
    values.set(world.getComponents(), cv::Enemy{});  // registered with the default options
    EXPECT_EQ(values.getPresence(0), rtype::ecs::Presence::kServerOnly);
    EXPECT_EQ(values.getPresence(1), rtype::ecs::Presence::kBoth);
}
