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

#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/exceptions/PrefabException.hpp"
#include "rtype/ecs/prefab/ComponentValues.hpp"
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

namespace rtype::test::component_values {
struct Health {
    std::int32_t value;
};
RTYPE_ECS_COMPONENT(Health, "test.values.Health", value);

struct Enemy {};
RTYPE_ECS_COMPONENT(Enemy, "test.values.Enemy");

/// @brief A component that owns memory and counts its instances.
struct Tracked {
    std::vector<std::int32_t> path;
    Counter counter;
};
RTYPE_ECS_COMPONENT(Tracked, "test.values.Tracked");
}  // namespace rtype::test::component_values

namespace cv = rtype::test::component_values;

namespace {
/// @return The bytes of a value, as the Luau bridge passes them.
template <typename T>
std::span<const std::byte> bytesOf(const T& value) {
    return std::as_bytes(std::span{&value, 1});
}
}  // namespace

TEST(ComponentValues, StartsEmpty) {
    const rtype::ecs::World world;
    const rtype::ecs::ComponentValues values;
    EXPECT_EQ(values.getCount(), 0U);
    EXPECT_FALSE(values.has(0));
    EXPECT_TRUE(values.get(0).empty());
    EXPECT_EQ(values.get<cv::Health>(world.getComponents()), nullptr);
}

TEST(ComponentValues, SetsACppValueAndRegistersItsType) {
    rtype::ecs::World world;
    rtype::ecs::ComponentValues values;
    values.set(world.getComponents(), cv::Health{.value = 3});
    const auto id = world.getComponents().find("test.values.Health");
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
    rtype::ecs::ComponentValues values;
    values.set(world.getComponents(), id, bytesOf(cv::Health{.value = 7}));
    EXPECT_EQ(values.get<cv::Health>(world.getComponents())->value, 7);
    EXPECT_EQ(values.get(id).size(), sizeof(cv::Health));
}

TEST(ComponentValues, ReplacesAValueSetAgainInPlace) {
    rtype::ecs::World world;
    rtype::ecs::ComponentValues values;
    values.set(world.getComponents(), cv::Health{.value = 1});
    values.set(world.getComponents(), cv::Enemy{});
    values.set(world.getComponents(), cv::Health{.value = 2});
    EXPECT_EQ(values.getCount(), 2U);
    EXPECT_EQ(values.get<cv::Health>(world.getComponents())->value, 2);
    EXPECT_EQ(values.getId(0), *world.getComponents().find("test.values.Health"));  // keeps its first place
}

TEST(ComponentValues, HoldsTags) {
    rtype::ecs::World world;
    rtype::ecs::ComponentValues values;
    values.set(world.getComponents(), cv::Enemy{});
    const rtype::ecs::ComponentId id = *world.getComponents().find("test.values.Enemy");
    EXPECT_TRUE(values.has(id));
    EXPECT_TRUE(values.get(id).empty());
    EXPECT_TRUE(values.getValue(0).empty());
}

TEST(ComponentValues, CopiesAndDestroysValuesThatOwnMemory) {
    liveTracked = 0;
    {
        rtype::ecs::World world;
        rtype::ecs::ComponentValues values;
        values.set(world.getComponents(), cv::Tracked{.path = {1, 2, 3}, .counter = {}});
        EXPECT_EQ(liveTracked, 1);
        const cv::Tracked source{.path = {4, 5}, .counter = {}};
        values.set(world.getComponents(), *world.getComponents().find("test.values.Tracked"), bytesOf(source));
        EXPECT_EQ(liveTracked, 2);  // the source and the copy; the replaced value is gone
        EXPECT_EQ(values.get<cv::Tracked>(world.getComponents())->path, (std::vector<std::int32_t>{4, 5}));
        rtype::ecs::ComponentValues moved{std::move(values)};
        EXPECT_EQ(liveTracked, 2);
        EXPECT_TRUE(moved.remove(*world.getComponents().find("test.values.Tracked")));
        EXPECT_EQ(liveTracked, 1);
    }
    EXPECT_EQ(liveTracked, 0);
}

TEST(ComponentValues, RemovesAValue) {
    rtype::ecs::World world;
    rtype::ecs::ComponentValues values;
    values.set(world.getComponents(), cv::Health{.value = 1});
    const rtype::ecs::ComponentId id = *world.getComponents().find("test.values.Health");
    EXPECT_TRUE(values.remove(id));
    EXPECT_FALSE(values.has(id));
    EXPECT_FALSE(values.remove(id));
}

TEST(ComponentValues, RefusesAnUnknownIdOrBytesOfTheWrongSize) {
    rtype::ecs::World world;
    const rtype::ecs::ComponentId id = world.getComponents().registerType<cv::Health>();
    rtype::ecs::ComponentValues values;
    const std::int64_t tooBig = 0;
    EXPECT_THROW(values.set(world.getComponents(), 99, bytesOf(cv::Health{})), rtype::ecs::exceptions::PrefabException);
    try {
        values.set(world.getComponents(), id, bytesOf(tooBig));
        FAIL() << "no PrefabException";
    } catch (const rtype::ecs::exceptions::PrefabException& error) {
        EXPECT_EQ(std::string{error.what()}, "Component 'test.values.Health' takes 4 bytes, not 8");
    }
    EXPECT_EQ(values.getCount(), 0U);
}
