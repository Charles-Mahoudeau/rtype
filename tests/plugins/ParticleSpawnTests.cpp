/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSpawnTests
*/

#include <gtest/gtest.h>

#include <cstdint>

#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/particle/ParticleSpawn.hpp"
#include "plugins/particle/ParticleSystems.hpp"

using rtype::plugins::backend::graphics::ParticleBurst;
using rtype::plugins::backend::graphics::ParticleSystemDesc;
using rtype::plugins::backend::graphics::ParticleSystemId;
using rtype::plugins::particle::computeSpawnCount;
using rtype::plugins::particle::EmitterState;
using rtype::plugins::particle::isEmissionOver;

namespace {
/// @return The state of an emitter that just started.
EmitterState makeState() { return EmitterState{.simulation = ParticleSystemId{}}; }

/// @return A desc that emits @p rate particles per second, in cycles of @p duration seconds.
ParticleSystemDesc makeDesc(float rate, float duration, bool loop) {
    ParticleSystemDesc desc;
    desc.rate = rate;
    desc.duration = duration;
    desc.loop = loop;
    return desc;
}

/// @return The particles emitted over @p frames frames of @p deltaTime seconds.
std::uint32_t countOver(const ParticleSystemDesc& desc, EmitterState& state, int frames, float deltaTime) {
    std::uint32_t total = 0;
    for (int i = 0; i < frames; ++i) {
        total += computeSpawnCount(desc, 1.0F, deltaTime, state);
    }
    return total;
}
}  // namespace

TEST(ParticleSpawnTests, RateEmitsItsParticlesPerSecond) {
    const ParticleSystemDesc desc = makeDesc(10.0F, 1.0F, true);
    EmitterState state = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.5F, state), 5U);
    EXPECT_FLOAT_EQ(state.time, 0.5F);
}

TEST(ParticleSpawnTests, RateCarriesFractionsToTheNextFrames) {
    const ParticleSystemDesc desc = makeDesc(10.0F, 100.0F, true);
    EmitterState state = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.05F, state), 0U);
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.05F, state), 1U);
    EXPECT_EQ(countOver(desc, state, 98, 0.05F), 49U);
}

TEST(ParticleSpawnTests, RateScaleMultipliesTheRate) {
    const ParticleSystemDesc desc = makeDesc(10.0F, 1.0F, true);
    EmitterState doubled = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 2.0F, 0.5F, doubled), 10U);
    EmitterState negative = makeState();
    EXPECT_EQ(computeSpawnCount(desc, -1.0F, 0.5F, negative), 0U);
}

TEST(ParticleSpawnTests, WithoutLoopTheRateStopsAfterTheDuration) {
    const ParticleSystemDesc desc = makeDesc(10.0F, 1.0F, false);
    EmitterState state = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.8F, state), 8U);
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.8F, state), 2U);
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.8F, state), 0U);
}

TEST(ParticleSpawnTests, ABurstFiresOnceAtItsTime) {
    ParticleSystemDesc desc = makeDesc(0.0F, 1.0F, false);
    desc.bursts.push_back(ParticleBurst{.time = 0.0F, .count = 30});
    EmitterState state = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.1F, state), 30U);
    EXPECT_EQ(countOver(desc, state, 20, 0.1F), 0U);
}

TEST(ParticleSpawnTests, ABurstLaterInTheCycleWaitsForIt) {
    ParticleSystemDesc desc = makeDesc(0.0F, 2.0F, false);
    desc.bursts.push_back(ParticleBurst{.time = 0.5F, .count = 4});
    EmitterState state = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.25F, state), 0U);
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.25F, state), 0U);
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.25F, state), 4U);
}

TEST(ParticleSpawnTests, ABurstRepeatsItsCyclesAtItsInterval) {
    ParticleSystemDesc desc = makeDesc(0.0F, 10.0F, false);
    desc.bursts.push_back(ParticleBurst{.time = 0.0F, .count = 2, .cycles = 3, .interval = 1.0F});
    EmitterState state = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.5F, state), 2U);
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 1.0F, state), 2U);
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 1.0F, state), 2U);
    EXPECT_EQ(countOver(desc, state, 10, 1.0F), 0U);
}

TEST(ParticleSpawnTests, BurstsBeyondTheDurationNeverFire) {
    ParticleSystemDesc desc = makeDesc(0.0F, 1.0F, true);
    desc.bursts.push_back(ParticleBurst{.time = 0.0F, .count = 1, .cycles = 5, .interval = 0.4F});
    EmitterState state = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.999F, state), 3U);
}

TEST(ParticleSpawnTests, LoopingRepeatsTheBurstsEveryCycle) {
    ParticleSystemDesc desc = makeDesc(0.0F, 1.0F, true);
    desc.bursts.push_back(ParticleBurst{.time = 0.0F, .count = 5});
    EmitterState state = makeState();
    EXPECT_EQ(countOver(desc, state, 30, 0.1F), 15U);
}

TEST(ParticleSpawnTests, ALongFrameCountsEveryCycleItCovers) {
    ParticleSystemDesc desc = makeDesc(4.0F, 1.0F, true);
    desc.bursts.push_back(ParticleBurst{.time = 0.5F, .count = 10});
    EmitterState state = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 3.0F, state), 12U + 30U);
}

TEST(ParticleSpawnTests, ANullFrameEmitsNothing) {
    ParticleSystemDesc desc = makeDesc(10.0F, 1.0F, true);
    desc.bursts.push_back(ParticleBurst{.time = 0.0F, .count = 5});
    EmitterState state = makeState();
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, 0.0F, state), 0U);
    EXPECT_EQ(computeSpawnCount(desc, 1.0F, -1.0F, state), 0U);
    EXPECT_FLOAT_EQ(state.time, 0.0F);
}

TEST(ParticleSpawnTests, EmissionIsOverAfterTheOneCycleWithoutLoop) {
    EXPECT_FALSE(isEmissionOver(makeDesc(0.0F, 1.0F, false), 0.5F));
    EXPECT_TRUE(isEmissionOver(makeDesc(0.0F, 1.0F, false), 1.0F));
    EXPECT_FALSE(isEmissionOver(makeDesc(0.0F, 1.0F, true), 100.0F));
}
