/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** CpuSimulationTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/geometric.hpp>
#include <numbers>

#include "plugins/backend/exceptions/ParticleExceptions.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/particle/CpuSimulation.hpp"

using rtype::plugins::backend::graphics::CircleShape;
using rtype::plugins::backend::graphics::ParticleEmission;
using rtype::plugins::backend::graphics::ParticleSpace;
using rtype::plugins::backend::graphics::ParticleSystemDesc;
using rtype::plugins::backend::graphics::RectShape;
using rtype::plugins::particle::CpuSimulation;

namespace {
constexpr float kTolerance = 1e-4F;

/// @return A desc whose particles live 1 second, go right at 10 units per second, and do nothing else.
ParticleSystemDesc makeDesc() {
    ParticleSystemDesc desc;
    desc.capacity = 16;
    desc.lifetime = {.min = 1.0F, .max = 1.0F};
    desc.speed = {.min = 10.0F, .max = 10.0F};
    desc.angle = {.min = 0.0F, .max = 0.0F};
    return desc;
}

/// @return An emission of @p spawnCount particles at @p position, advancing @p deltaTime seconds.
ParticleEmission makeEmission(std::uint32_t spawnCount, float deltaTime, glm::vec2 position = {0.0F, 0.0F}) {
    return ParticleEmission{.position = position, .deltaTime = deltaTime, .seed = 7, .spawnCount = spawnCount};
}
}  // namespace

TEST(CpuSimulationTests, StartsEmpty) {
    const CpuSimulation simulation{makeDesc()};
    EXPECT_EQ(simulation.getCount(), 0U);
    EXPECT_TRUE(simulation.getInstances().empty());
    EXPECT_EQ(simulation.getCapacity(), 16U);
}

TEST(CpuSimulationTests, ThrowsOnAnInvalidDesc) {
    ParticleSystemDesc desc = makeDesc();
    desc.capacity = 0;
    EXPECT_THROW(CpuSimulation{desc}, rtype::plugins::backend::exceptions::ParticleException);
}

TEST(CpuSimulationTests, EmitsTheSpawnCount) {
    CpuSimulation simulation{makeDesc()};
    simulation.step(makeEmission(5, 0.0F));
    EXPECT_EQ(simulation.getCount(), 5U);
    EXPECT_EQ(simulation.getInstances().size(), 5U);
}

TEST(CpuSimulationTests, DropsWhatIsBeyondTheCapacity) {
    CpuSimulation simulation{makeDesc()};
    simulation.step(makeEmission(10, 0.0F));
    simulation.step(makeEmission(10, 0.0F));
    EXPECT_EQ(simulation.getCount(), 16U);
}

TEST(CpuSimulationTests, MovesAlongTheAngleFromTheEmitter) {
    CpuSimulation simulation{makeDesc()};
    simulation.step(makeEmission(1, 0.5F, {100.0F, 50.0F}));
    const auto& instance = simulation.getInstances()[0];
    EXPECT_NEAR(instance.position.x, 105.0F, kTolerance);
    EXPECT_NEAR(instance.position.y, 50.0F, kTolerance);
    EXPECT_NEAR(instance.velocity.x, 10.0F, kTolerance);
    EXPECT_NEAR(instance.lifetime, 0.5F, kTolerance);
}

TEST(CpuSimulationTests, EmitterRotationTurnsTheDirection) {
    CpuSimulation simulation{makeDesc()};
    ParticleEmission emission = makeEmission(1, 1.0F / 2.0F);
    emission.rotation = std::numbers::pi_v<float> / 2.0F;
    simulation.step(emission);
    const auto& instance = simulation.getInstances()[0];
    EXPECT_NEAR(instance.position.x, 0.0F, kTolerance);
    EXPECT_NEAR(instance.position.y, 5.0F, kTolerance);
}

TEST(CpuSimulationTests, ParticlesDieAtTheEndOfTheirLife) {
    CpuSimulation simulation{makeDesc()};
    simulation.step(makeEmission(3, 0.6F));
    simulation.step(makeEmission(2, 0.0F));
    EXPECT_EQ(simulation.getCount(), 5U);
    simulation.step(makeEmission(0, 0.5F));
    EXPECT_EQ(simulation.getCount(), 2U);
    for (const auto& instance : simulation.getInstances()) {
        EXPECT_NEAR(instance.lifetime, 0.5F, kTolerance);
    }
    simulation.step(makeEmission(0, 0.5F));
    EXPECT_EQ(simulation.getCount(), 0U);
}

TEST(CpuSimulationTests, GravityAcceleratesTheParticles) {
    ParticleSystemDesc desc = makeDesc();
    desc.speed = {.min = 0.0F, .max = 0.0F};
    desc.gravity = {0.0F, 20.0F};
    CpuSimulation simulation{desc};
    simulation.step(makeEmission(1, 0.5F));
    EXPECT_NEAR(simulation.getInstances()[0].velocity.y, 10.0F, kTolerance);
    EXPECT_NEAR(simulation.getInstances()[0].position.y, 5.0F, kTolerance);
}

TEST(CpuSimulationTests, DragSlowsTheParticlesWhateverTheFrameRate) {
    ParticleSystemDesc desc = makeDesc();
    desc.drag = 2.0F;
    CpuSimulation oneStep{desc};
    oneStep.step(makeEmission(1, 0.0F));
    oneStep.step(makeEmission(0, 0.5F));
    CpuSimulation twoSteps{desc};
    twoSteps.step(makeEmission(1, 0.0F));
    twoSteps.step(makeEmission(0, 0.25F));
    twoSteps.step(makeEmission(0, 0.25F));
    const float expected = 10.0F * std::exp(-2.0F * 0.5F);
    EXPECT_NEAR(oneStep.getInstances()[0].velocity.x, expected, kTolerance);
    EXPECT_NEAR(twoSteps.getInstances()[0].velocity.x, expected, kTolerance);
}

TEST(CpuSimulationTests, CurvesShapeColourSizeAndSpeedOverLife) {
    ParticleSystemDesc desc = makeDesc();
    desc.size = {.min = 4.0F, .max = 4.0F};
    desc.colorOverLife.keys = {{.t = 0.0F, .value = {1.0F, 1.0F, 1.0F, 1.0F}},
                               {.t = 1.0F, .value = {1.0F, 0.0F, 0.0F, 0.0F}}};
    desc.sizeOverLife.keys = {{.t = 0.0F, .value = 1.0F}, {.t = 1.0F, .value = 0.0F}};
    desc.speedOverLife.keys = {{.t = 0.0F, .value = 2.0F}, {.t = 1.0F, .value = 2.0F}};
    CpuSimulation simulation{desc};
    simulation.step(makeEmission(1, 0.5F));
    const auto& instance = simulation.getInstances()[0];
    EXPECT_NEAR(instance.color.y, 0.5F, 0.02F);
    EXPECT_NEAR(instance.color.w, 0.5F, 0.02F);
    EXPECT_NEAR(instance.size.x, 2.0F, 0.05F);
    EXPECT_NEAR(instance.velocity.x, 20.0F, kTolerance);
    EXPECT_NEAR(instance.position.x, 10.0F, kTolerance);
}

TEST(CpuSimulationTests, RotationTurnsWithTheAngularVelocity) {
    ParticleSystemDesc desc = makeDesc();
    desc.rotation = {.min = 1.0F, .max = 1.0F};
    desc.angularVelocity = {.min = 2.0F, .max = 2.0F};
    CpuSimulation simulation{desc};
    simulation.step(makeEmission(1, 0.25F));
    EXPECT_NEAR(simulation.getInstances()[0].rotation, 1.5F, kTolerance);
}

TEST(CpuSimulationTests, RangesStayWithinTheirBounds) {
    ParticleSystemDesc desc = makeDesc();
    desc.capacity = 256;
    desc.lifetime = {.min = 1.0F, .max = 3.0F};
    desc.size = {.min = 2.0F, .max = 5.0F};
    CpuSimulation simulation{desc};
    simulation.step(makeEmission(256, 0.0F));
    for (const auto& instance : simulation.getInstances()) {
        EXPECT_GE(instance.lifetime, 1.0F);
        EXPECT_LT(instance.lifetime, 3.0F);
        EXPECT_GE(instance.size.x, 2.0F);
        EXPECT_LT(instance.size.x, 5.0F);
    }
}

TEST(CpuSimulationTests, CircleShapeSpawnsInsideOrOnItsEdge) {
    ParticleSystemDesc desc = makeDesc();
    desc.capacity = 128;
    desc.speed = {.min = 0.0F, .max = 0.0F};
    desc.shape = CircleShape{.radius = 10.0F};
    CpuSimulation inside{desc};
    inside.step(makeEmission(128, 0.0F, {50.0F, 50.0F}));
    for (const auto& instance : inside.getInstances()) {
        EXPECT_LE(glm::length(instance.position - glm::vec2{50.0F, 50.0F}), 10.0F + kTolerance);
    }
    desc.shape = CircleShape{.radius = 10.0F, .edgeOnly = true};
    CpuSimulation edge{desc};
    edge.step(makeEmission(128, 0.0F));
    for (const auto& instance : edge.getInstances()) {
        EXPECT_NEAR(glm::length(instance.position), 10.0F, kTolerance);
    }
}

TEST(CpuSimulationTests, RectShapeIsScaledByTheEmitter) {
    ParticleSystemDesc desc = makeDesc();
    desc.capacity = 128;
    desc.speed = {.min = 0.0F, .max = 0.0F};
    desc.shape = RectShape{.size = {20.0F, 10.0F}};
    CpuSimulation simulation{desc};
    ParticleEmission emission = makeEmission(128, 0.0F);
    emission.scale = {2.0F, 1.0F};
    simulation.step(emission);
    for (const auto& instance : simulation.getInstances()) {
        EXPECT_LE(std::abs(instance.position.x), 20.0F);
        EXPECT_LE(std::abs(instance.position.y), 5.0F);
    }
}

TEST(CpuSimulationTests, SameSeedAndParticlesGiveTheSameResult) {
    ParticleSystemDesc desc = makeDesc();
    desc.speed = {.min = 1.0F, .max = 100.0F};
    desc.angle = {.min = 0.0F, .max = 6.0F};
    CpuSimulation first{desc};
    CpuSimulation second{desc};
    first.step(makeEmission(8, 0.1F));
    second.step(makeEmission(8, 0.1F));
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(first.getInstances()[i].position, second.getInstances()[i].position);
    }
}

TEST(CpuSimulationTests, FirstParticleGivesEachParticleItsOwnRandomNumbers) {
    ParticleSystemDesc desc = makeDesc();
    desc.speed = {.min = 1.0F, .max = 100.0F};
    CpuSimulation simulation{desc};
    ParticleEmission emission = makeEmission(1, 0.1F);
    simulation.step(emission);
    emission.firstParticle = 1;
    simulation.step(emission);
    EXPECT_NE(simulation.getInstances()[0].velocity.x, simulation.getInstances()[1].velocity.x);

    CpuSimulation together{desc};
    together.step(makeEmission(2, 0.1F));
    EXPECT_NEAR(together.getInstances()[1].velocity.x, simulation.getInstances()[1].velocity.x, kTolerance);
}

TEST(CpuSimulationTests, LocalParticlesFollowTheEmitter) {
    ParticleSystemDesc desc = makeDesc();
    desc.space = ParticleSpace::kLocal;
    CpuSimulation local{desc};
    local.step(makeEmission(1, 0.0F, {0.0F, 0.0F}));
    local.step(makeEmission(0, 0.5F, {100.0F, 0.0F}));
    EXPECT_NEAR(local.getInstances()[0].position.x, 105.0F, kTolerance);

    CpuSimulation world{makeDesc()};
    world.step(makeEmission(1, 0.0F, {0.0F, 0.0F}));
    world.step(makeEmission(0, 0.5F, {100.0F, 0.0F}));
    EXPECT_NEAR(world.getInstances()[0].position.x, 5.0F, kTolerance);
}

TEST(CpuSimulationTests, ClearKillsEveryParticle) {
    CpuSimulation simulation{makeDesc()};
    simulation.step(makeEmission(4, 0.0F));
    simulation.clear();
    EXPECT_EQ(simulation.getCount(), 0U);
    EXPECT_TRUE(simulation.getInstances().empty());
}
