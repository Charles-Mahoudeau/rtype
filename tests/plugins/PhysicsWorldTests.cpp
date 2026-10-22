/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsWorldTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <thread>
#include <utility>

#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/detail/JoltRuntime.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"

using rtype::plugins::physics::PhysicsSettings;
using rtype::plugins::physics::PhysicsWorld;
using rtype::plugins::physics::detail::JoltRuntime;
using rtype::plugins::physics::exceptions::PhysicsException;

namespace {
constexpr float kFixedDelta = 1.0F / 60.0F;
constexpr float kTolerance = 1e-3F;
constexpr std::uint64_t kStepsPerThread = 120;

/// @brief Small capacities, so tests build worlds quickly.
PhysicsSettings smallSettings() {
    return PhysicsSettings{
        .maxBodies = 16, .maxBodyPairs = 16, .maxContactConstraints = 16, .tempAllocatorBytes = 1024 * 1024};
}
}  // namespace

TEST(PhysicsWorld, SetsJoltUpWhileItLives) {
    {
        const PhysicsWorld world{smallSettings()};
        EXPECT_TRUE(JoltRuntime::isSetUp());
    }
    EXPECT_FALSE(JoltRuntime::isSetUp());
}

TEST(PhysicsWorld, StartsEmpty) {
    const PhysicsWorld world{smallSettings()};
    EXPECT_EQ(world.getBodyCount(), 0U);
    EXPECT_EQ(world.getStepCount(), 0U);
    EXPECT_FALSE(world.didLastStepDropContacts());
}

TEST(PhysicsWorld, KeepsTheGravityInWorldUnits) {
    PhysicsSettings settings = smallSettings();
    settings.gravity = {0.0F, 980.0F};
    settings.unitsPerMeter = 100.0F;
    PhysicsWorld world{settings};
    EXPECT_NEAR(world.getGravity().x, 0.0F, kTolerance);
    EXPECT_NEAR(world.getGravity().y, 980.0F, kTolerance);
    EXPECT_FLOAT_EQ(world.getUnitsPerMeter(), 100.0F);

    world.setGravity({-32.0F, 64.0F});
    EXPECT_NEAR(world.getGravity().x, -32.0F, kTolerance);
    EXPECT_NEAR(world.getGravity().y, 64.0F, kTolerance);
}

TEST(PhysicsWorld, StepsWithoutBodies) {
    PhysicsWorld world{smallSettings()};
    world.step(kFixedDelta);
    world.step(kFixedDelta);
    EXPECT_EQ(world.getStepCount(), 2U);
    EXPECT_EQ(world.getBodyCount(), 0U);
    EXPECT_FALSE(world.didLastStepDropContacts());
}

TEST(PhysicsWorld, MovingKeepsTheWorldAndJoltAlive) {
    PhysicsWorld first{smallSettings()};
    first.step(kFixedDelta);
    PhysicsWorld moved{std::move(first)};
    EXPECT_TRUE(JoltRuntime::isSetUp());
    moved.step(kFixedDelta);
    EXPECT_EQ(moved.getStepCount(), 2U);
}

TEST(PhysicsWorld, MoveAssigningReleasesTheOldWorld) {
    PhysicsWorld target{smallSettings()};
    PhysicsWorld source{smallSettings()};
    source.step(kFixedDelta);
    target = std::move(source);
    EXPECT_EQ(target.getStepCount(), 1U);
    target.step(kFixedDelta);
    EXPECT_EQ(target.getStepCount(), 2U);
}

TEST(PhysicsWorld, RejectsInvalidSettings) {
    PhysicsSettings noScale = smallSettings();
    noScale.unitsPerMeter = 0.0F;
    EXPECT_THROW(PhysicsWorld{noScale}, PhysicsException);

    PhysicsSettings negativeScale = smallSettings();
    negativeScale.unitsPerMeter = -1.0F;
    EXPECT_THROW(PhysicsWorld{negativeScale}, PhysicsException);

    PhysicsSettings noBodies = smallSettings();
    noBodies.maxBodies = 0;
    EXPECT_THROW(PhysicsWorld{noBodies}, PhysicsException);

    PhysicsSettings noScratch = smallSettings();
    noScratch.tempAllocatorBytes = 0;
    EXPECT_THROW(PhysicsWorld{noScratch}, PhysicsException);

    EXPECT_FALSE(JoltRuntime::isSetUp());
}

TEST(PhysicsWorld, WorldsOnSeveralThreadsStepIndependently) {
    std::array<std::uint64_t, 2> steps{};
    {
        std::array<std::jthread, 2> threads;
        for (std::size_t index = 0; index < threads.size(); ++index) {
            threads.at(index) = std::jthread{[&steps, index] {
                PhysicsWorld world{smallSettings()};
                for (std::uint64_t step = 0; step < kStepsPerThread; ++step) {
                    world.step(kFixedDelta);
                }
                steps.at(index) = world.getStepCount();
            }};
        }
    }
    EXPECT_EQ(steps.at(0), kStepsPerThread);
    EXPECT_EQ(steps.at(1), kStepsPerThread);
    EXPECT_FALSE(JoltRuntime::isSetUp());
}
