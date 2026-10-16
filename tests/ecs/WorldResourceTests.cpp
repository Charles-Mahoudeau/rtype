/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** WorldResourceTests
*/

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "rtype/ecs/change/ComponentTicks.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/change/Tick.hpp"
#include "rtype/ecs/component/ComponentInfo.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/error/Error.hpp"
#include "rtype/ecs/error/Result.hpp"
#include "rtype/ecs/exceptions/ComponentExceptions.hpp"
#include "rtype/ecs/exceptions/MissingResourceException.hpp"
#include "rtype/ecs/exceptions/ResourceExceptions.hpp"
#include "rtype/ecs/resource/ResourceInfo.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"
#include "rtype/ecs/storage/ViewAs.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
/// @brief Number of Counter instances alive, to detect leaks and double destruction.
int liveCounters = 0;

/// @brief Counts its own instances.
struct Counter {
    Counter() noexcept { ++liveCounters; }
    ~Counter() { --liveCounters; }
    Counter(const Counter& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(const Counter& /*other*/) noexcept = default;
    Counter(Counter&& /*other*/) noexcept { ++liveCounters; }
    Counter& operator=(Counter&& /*other*/) noexcept = default;
};
}  // namespace

namespace rtype::test::world_resource {
struct Time {
    float delta;
    float fixedDelta;
};
RTYPE_ECS_RESOURCE(Time, "test.worldres.Time");

struct Backend {
    std::unique_ptr<std::int32_t> window;
};
RTYPE_ECS_RESOURCE(Backend, "test.worldres.Backend");

struct Assets {
    std::vector<std::int32_t> textures;
    Counter counter;
};
RTYPE_ECS_RESOURCE(Assets, "test.worldres.Assets");

struct Paused {};
RTYPE_ECS_RESOURCE(Paused, "test.worldres.Paused");

/// @brief A component under Time's name: refused, since a name is either a component or a resource.
struct TimeComponent {
    float value;
};
RTYPE_ECS_COMPONENT(TimeComponent, "test.worldres.Time", value);
}  // namespace rtype::test::world_resource

namespace wres = rtype::test::world_resource;

namespace {
/// @brief The C++ view of a Score value, which only exists as a run-time description in the world.
struct ScoreValue {
    std::int32_t points;
};

/// @return The bytes of a value, as the Luau bridge passes them.
template <typename T>
std::span<const std::byte> bytesOf(const T& value) {
    return std::as_bytes(std::span{&value, 1});
}

/// @return A run-time description, as a Luau script would provide it.
rtype::ecs::ResourceInfo makeScore() {
    return rtype::ecs::ResourceInfo{.type = {.name = "test.worldres.Score", .size = 4, .alignment = 4}};
}
}  // namespace

TEST(WorldResource, StartsWithoutResources) {
    const rtype::ecs::World world;
    EXPECT_EQ(world.getResources().getRegistry().getCount(), 0U);
    EXPECT_FALSE(world.getResources().has<wres::Time>());
    EXPECT_EQ(world.getResources().get<wres::Time>(), nullptr);
    EXPECT_FALSE(world.getResources().find("test.worldres.Time"));
}

TEST(WorldResource, InsertsAndReadsACppResource) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 0.5F, .fixedDelta = 0.25F});
    ASSERT_TRUE(world.getResources().has<wres::Time>());
    const wres::Time* time = world.getResources().get<wres::Time>();
    ASSERT_NE(time, nullptr);
    EXPECT_FLOAT_EQ(time->delta, 0.5F);
    EXPECT_FLOAT_EQ(time->fixedDelta, 0.25F);
    EXPECT_FALSE(world.getResources().getRegistry().find<wres::Time>()->nonSend);
}

TEST(WorldResource, InsertReplacesTheValue) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 1.F, .fixedDelta = 0.F});
    world.getResources().insert(wres::Time{.delta = 2.F, .fixedDelta = 0.F});
    EXPECT_FLOAT_EQ(world.getResources().get<wres::Time>()->delta, 2.F);
    EXPECT_EQ(world.getResources().getRegistry().getCount(), 1U);
}

TEST(WorldResource, RegisteringWithoutAValueLeavesItEmpty) {
    rtype::ecs::World world;
    const rtype::ecs::ResourceId id = world.getResources().registerType<wres::Time>();
    EXPECT_EQ(world.getResources().find("test.worldres.Time"), id);
    EXPECT_FALSE(world.getResources().has<wres::Time>());
    EXPECT_FALSE(world.getResources().getMut<wres::Time>());
    EXPECT_FALSE(world.getResources().getTicks<wres::Time>());
}

TEST(WorldResource, RemovesAResourceButKeepsItRegistered) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 1.F, .fixedDelta = 0.F});
    EXPECT_TRUE(world.getResources().remove<wres::Time>());
    EXPECT_FALSE(world.getResources().has<wres::Time>());
    EXPECT_EQ(world.getResources().get<wres::Time>(), nullptr);
    EXPECT_FALSE(world.getResources().remove<wres::Time>());
    EXPECT_TRUE(world.getResources().find("test.worldres.Time"));
}

TEST(WorldResource, StoresMoveOnlyResources) {
    rtype::ecs::World world;
    world.getResources().insertNonSend(wres::Backend{.window = std::make_unique<std::int32_t>(42)});
    const wres::Backend* backend = world.getResources().get<wres::Backend>();
    ASSERT_NE(backend, nullptr);
    ASSERT_NE(backend->window, nullptr);
    EXPECT_EQ(*backend->window, 42);
    EXPECT_TRUE(world.getResources().getRegistry().find<wres::Backend>()->nonSend);
}

TEST(WorldResource, RefusesToSwitchTheMainThreadFlag) {
    rtype::ecs::World world;
    world.getResources().insertNonSend(wres::Backend{});
    EXPECT_THROW(world.getResources().insert(wres::Backend{}), rtype::ecs::exceptions::ResourceRegistrationException);
    world.getResources().insert(wres::Time{});
    EXPECT_THROW(world.getResources().insertNonSend(wres::Time{}),
                 rtype::ecs::exceptions::ResourceRegistrationException);
    EXPECT_THROW(world.getResources().registerType<wres::Time>({.nonSend = true}),
                 rtype::ecs::exceptions::ResourceRegistrationException);
}

TEST(WorldResource, DestroysValuesWhenReplacedRemovedAndDestroyed) {
    liveCounters = 0;
    {
        rtype::ecs::World world;
        world.getResources().insert(wres::Assets{.textures = {1, 2}, .counter = {}});
        EXPECT_EQ(liveCounters, 1);
        world.getResources().insert(wres::Assets{.textures = {3}, .counter = {}});
        EXPECT_EQ(liveCounters, 1);
        EXPECT_EQ(world.getResources().get<wres::Assets>()->textures.size(), 1U);
        world.getResources().remove<wres::Assets>();
        EXPECT_EQ(liveCounters, 0);
        world.getResources().insert(wres::Assets{});
        EXPECT_EQ(liveCounters, 1);
    }
    EXPECT_EQ(liveCounters, 0);
}

TEST(WorldResource, StoresEmptyResources) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Paused{});
    EXPECT_TRUE(world.getResources().has<wres::Paused>());
    EXPECT_TRUE(world.getResources().remove<wres::Paused>());
    EXPECT_FALSE(world.getResources().has<wres::Paused>());
}

TEST(WorldResource, ANameUsedByAComponentCannotBeAResource) {
    rtype::ecs::World world;
    world.getComponents().registerType<wres::TimeComponent>();
    try {
        world.getResources().insert(wres::Time{.delta = 3.F, .fixedDelta = 0.F});
        FAIL() << "expected the resource to be refused";
    } catch (const rtype::ecs::exceptions::ResourceRegistrationException& error) {
        EXPECT_NE(std::string{error.what()}.find("already used by a component"), std::string::npos) << error.what();
    }
    EXPECT_FALSE(world.getResources().find("test.worldres.Time"));
    EXPECT_TRUE(world.getComponents().find("test.worldres.Time"));
}

TEST(WorldResource, ANameUsedByAResourceCannotBeAComponent) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 3.F, .fixedDelta = 0.F});
    EXPECT_THROW(world.getComponents().registerType<wres::TimeComponent>(),
                 rtype::ecs::exceptions::ComponentRegistrationException);
    const rtype::ecs::Entity entity = world.spawn();
    EXPECT_THROW(world.getComponents().add(entity, wres::TimeComponent{}),
                 rtype::ecs::exceptions::ComponentRegistrationException);
    EXPECT_FALSE(world.getComponents().find("test.worldres.Time"));
    EXPECT_FLOAT_EQ(world.getResources().get<wres::Time>()->delta, 3.F);  // the resource is untouched
}

TEST(WorldResource, RunTimeRegistrationsRefuseTheOtherKindsNames) {
    rtype::ecs::World world;
    world.getComponents().registerType<wres::TimeComponent>();
    world.getResources().insert(wres::Paused{});
    const rtype::ecs::Result<rtype::ecs::ResourceId> resource = world.getResources().registerType(
        rtype::ecs::ResourceInfo{.type = {.name = "test.worldres.Time", .size = 4, .alignment = 4}});
    ASSERT_FALSE(resource);
    EXPECT_EQ(resource.error().getKind(), rtype::ecs::ErrorKind::kNameCollision);
    EXPECT_EQ(std::string{resource.error().getMessage()},
              "Resource 'test.worldres.Time': the name is already used by a component");
    const rtype::ecs::Result<rtype::ecs::ComponentId> component = world.getComponents().registerType(
        rtype::ecs::ComponentInfo{.type = {.name = "test.worldres.Paused", .size = 4, .alignment = 4}});
    ASSERT_FALSE(component);
    EXPECT_EQ(component.error().getKind(), rtype::ecs::ErrorKind::kNameCollision);
    EXPECT_EQ(std::string{component.error().getMessage()},
              "Component 'test.worldres.Paused': the name is already used by a resource");
}

TEST(WorldResource, WorldsAreIndependent) {
    rtype::ecs::World first;
    rtype::ecs::World second;
    first.getResources().insert(wres::Time{.delta = 1.F, .fixedDelta = 0.F});
    EXPECT_FALSE(second.getResources().has<wres::Time>());
    EXPECT_FALSE(second.getResources().find("test.worldres.Time"));
}

TEST(WorldResource, MovingKeepsResources) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 5.F, .fixedDelta = 0.F});
    const rtype::ecs::World moved{std::move(world)};
    ASSERT_TRUE(moved.getResources().has<wres::Time>());
    EXPECT_FLOAT_EQ(moved.getResources().get<wres::Time>()->delta, 5.F);
}

TEST(WorldResource, AMutStaysValidWhenOtherResourcesAreRegistered) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 0.F, .fixedDelta = 0.F});
    rtype::ecs::Mut<wres::Time> time = world.getResources().getMut<wres::Time>();
    for (int index = 0; index < 64; ++index) {
        ASSERT_TRUE(world.getResources().registerType(rtype::ecs::ResourceInfo{
            .type = {.name = std::format("test.worldres.Many{}", index), .size = 4, .alignment = 4}}));
    }
    time->delta = 7.F;
    EXPECT_FLOAT_EQ(world.getResources().get<wres::Time>()->delta, 7.F);
}

TEST(WorldResource, RegistersRunTimeDescriptions) {
    rtype::ecs::World world;
    const rtype::ecs::Result<rtype::ecs::ResourceId> id = world.getResources().registerType(makeScore());
    ASSERT_TRUE(id);
    EXPECT_EQ(world.getResources().find("test.worldres.Score"), *id);
    EXPECT_FALSE(world.getResources().has(*id));
    const rtype::ecs::Result<rtype::ecs::ResourceId> invalid = world.getResources().registerType(
        rtype::ecs::ResourceInfo{.type = {.name = "test.worldres.Bad", .size = 4, .alignment = 3}});
    ASSERT_FALSE(invalid);
    EXPECT_EQ(invalid.error().getKind(), rtype::ecs::ErrorKind::kInvalidType);
}

TEST(WorldResource, InsertsAndReadsARunTimeResourceById) {
    rtype::ecs::World world;
    const rtype::ecs::ResourceId id = *world.getResources().registerType(makeScore());
    EXPECT_TRUE(world.getResources().insert(id, bytesOf(ScoreValue{.points = 100})));
    ASSERT_TRUE(world.getResources().has(id));
    EXPECT_EQ(rtype::ecs::storage::viewAs<ScoreValue>(world.getResources().get(id))->points, 100);
    EXPECT_TRUE(world.getResources().insert(id, bytesOf(ScoreValue{.points = 200})));
    EXPECT_EQ(rtype::ecs::storage::viewAs<ScoreValue>(world.getResources().get(id))->points, 200);
    EXPECT_TRUE(world.getResources().remove(id));
    EXPECT_FALSE(world.getResources().has(id));
    EXPECT_TRUE(world.getResources().get(id).empty());
}

TEST(WorldResource, RefusesInvalidInsertsById) {
    rtype::ecs::World world;
    const rtype::ecs::ResourceId score = *world.getResources().registerType(makeScore());
    const rtype::ecs::ResourceId backend = world.getResources().registerType<wres::Backend>();
    EXPECT_FALSE(world.getResources().insert(score, bytesOf(std::int64_t{1})));
    EXPECT_FALSE(world.getResources().insert(backend, bytesOf(wres::Backend{})));
    EXPECT_FALSE(world.getResources().insert(rtype::ecs::ResourceId{99}, bytesOf(ScoreValue{.points = 1})));
    EXPECT_FALSE(world.getResources().has(score));
    EXPECT_FALSE(world.getResources().has(backend));
}

TEST(WorldResource, AnswersNothingForAnUnknownId) {
    rtype::ecs::World world;
    const rtype::ecs::ResourceId unknown{7};
    EXPECT_FALSE(world.getResources().has(unknown));
    EXPECT_TRUE(world.getResources().get(unknown).empty());
    EXPECT_TRUE(world.getResources().getMut(unknown).empty());
    EXPECT_FALSE(world.getResources().remove(unknown));
    EXPECT_FALSE(world.getResources().getTicks(unknown));
}

TEST(WorldResource, TypedAndErasedCallsSeeTheSameData) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 1.F, .fixedDelta = 2.F});
    const rtype::ecs::ResourceId id = *world.getResources().find("test.worldres.Time");
    EXPECT_EQ(world.getResources().get(id).data(),
              reinterpret_cast<const std::byte*>(world.getResources().get<wres::Time>()));
    EXPECT_EQ(world.getResources().getTicks(id)->changed, world.getResources().getTicks<wres::Time>()->changed);
}

TEST(WorldResource, InsertingStampsBothTicksAndReplacingOnlyTheChangedTick) {
    rtype::ecs::World world;
    world.advanceChangeTick(3);
    world.getResources().insert(wres::Time{});
    std::optional<rtype::ecs::ComponentTicks> ticks = world.getResources().getTicks<wres::Time>();
    ASSERT_TRUE(ticks);
    EXPECT_EQ(ticks->added, 3U);
    EXPECT_EQ(ticks->changed, 3U);
    world.advanceChangeTick(2);
    world.getResources().insert(wres::Time{});
    ticks = world.getResources().getTicks<wres::Time>();
    EXPECT_EQ(ticks->added, 3U);
    EXPECT_EQ(ticks->changed, 5U);
}

TEST(WorldResource, ReadingThroughMutDoesNotMarkChanged) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 4.F, .fixedDelta = 0.F});
    world.advanceChangeTick();
    const rtype::ecs::Mut<wres::Time> time = world.getResources().getMut<wres::Time>();
    ASSERT_TRUE(time);
    EXPECT_FLOAT_EQ(time.get().delta, 4.F);
    EXPECT_EQ(world.getResources().getTicks<wres::Time>()->changed, 0U);
}

TEST(WorldResource, WritingThroughMutMarksChanged) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 0.F, .fixedDelta = 0.F});
    world.advanceChangeTick(4);
    rtype::ecs::Mut<wres::Time> time = world.getResources().getMut<wres::Time>();
    time->delta = 9.F;
    EXPECT_FLOAT_EQ(world.getResources().get<wres::Time>()->delta, 9.F);
    EXPECT_EQ(world.getResources().getTicks<wres::Time>()->added, 0U);
    EXPECT_EQ(world.getResources().getTicks<wres::Time>()->changed, 4U);
}

TEST(WorldResource, TypeErasedGetMutMarksOnAccess) {
    rtype::ecs::World world;
    const rtype::ecs::ResourceId id = *world.getResources().registerType(makeScore());
    world.getResources().insert(id, bytesOf(ScoreValue{.points = 1}));
    world.advanceChangeTick(6);
    const std::span<std::byte> bytes = world.getResources().getMut(id);
    ASSERT_FALSE(bytes.empty());
    EXPECT_EQ(world.getResources().getTicks(id)->changed, 6U);
}

TEST(WorldResource, ChangeDetectionSurvivesTheTickWrapping) {
    rtype::ecs::World world;
    world.advanceChangeTick();
    world.getResources().insert(wres::Time{});             // added and changed at tick 1
    rtype::ecs::Tick lastRun = world.advanceChangeTick();  // a system runs at tick 2 and sees it
    // Five billion ticks later, past the 32-bit wrap; the system keeps clamping its own last run, as it must.
    for (int step = 0; step < 5; ++step) {
        world.advanceChangeTick(1'000'000'000);
        rtype::ecs::clampTick(lastRun, world.getChangeTick());
    }
    const rtype::ecs::Tick thisRun = world.getChangeTick();
    const std::optional<rtype::ecs::ComponentTicks> untouched = world.getResources().getTicks<wres::Time>();
    ASSERT_TRUE(untouched);
    EXPECT_LE(thisRun - untouched->changed, rtype::ecs::kMaxChangeAge);  // clamped by the world
    EXPECT_FALSE(untouched->isChanged(lastRun, thisRun));                // still old, not "new" after the wrap
    world.getResources().getMut<wres::Time>()->delta = 1.F;              // now written
    EXPECT_TRUE(world.getResources().getTicks<wres::Time>()->isChanged(lastRun, thisRun));
}

TEST(WorldResource, RequireReturnsThePresentValue) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 1.5F, .fixedDelta = 0.F});
    EXPECT_FLOAT_EQ(world.getResources().require<wres::Time>().delta, 1.5F);
    EXPECT_EQ(&world.getResources().require<wres::Time>(), world.getResources().get<wres::Time>());
    EXPECT_FLOAT_EQ(world.getResources().requireMut<wres::Time>().get().delta, 1.5F);
}

TEST(WorldResource, RequireThrowsForAnUnregisteredResource) {
    rtype::ecs::World world;
    EXPECT_THROW(static_cast<void>(world.getResources().require<wres::Time>()),
                 rtype::ecs::exceptions::MissingResourceException);
    EXPECT_THROW(static_cast<void>(world.getResources().requireMut<wres::Time>()),
                 rtype::ecs::exceptions::MissingResourceException);
}

TEST(WorldResource, RequireThrowsForARegisteredResourceWithoutAValue) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{});
    world.getResources().remove<wres::Time>();
    EXPECT_THROW(static_cast<void>(world.getResources().require<wres::Time>()),
                 rtype::ecs::exceptions::MissingResourceException);
    EXPECT_THROW(static_cast<void>(world.getResources().requireMut<wres::Time>()),
                 rtype::ecs::exceptions::MissingResourceException);
}

TEST(WorldResource, MissingResourceExceptionNamesTheResource) {
    const rtype::ecs::World world;
    try {
        static_cast<void>(world.getResources().require<wres::Time>());
        FAIL() << "require did not throw";
    } catch (const rtype::ecs::exceptions::MissingResourceException& error) {
        EXPECT_NE(std::string_view{error.what()}.find("test.worldres.Time"), std::string_view::npos);
    }
}

TEST(WorldResource, RequireDoesNotMarkChangedButWritingThroughRequireMutDoes) {
    rtype::ecs::World world;
    world.getResources().insert(wres::Time{.delta = 0.F, .fixedDelta = 0.F});
    world.advanceChangeTick(3);
    static_cast<void>(world.getResources().require<wres::Time>());
    static_cast<void>(world.getResources().requireMut<wres::Time>().get());
    EXPECT_EQ(world.getResources().getTicks<wres::Time>()->changed, 0U);
    world.getResources().requireMut<wres::Time>()->delta = 2.F;
    EXPECT_FLOAT_EQ(world.getResources().get<wres::Time>()->delta, 2.F);
    EXPECT_EQ(world.getResources().getTicks<wres::Time>()->changed, 3U);
}
