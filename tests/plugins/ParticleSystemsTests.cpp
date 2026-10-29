/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticleSystemsTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <variant>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/ParticleSystemDesc.hpp"
#include "plugins/particle/CpuSimulation.hpp"
#include "plugins/particle/ParticleSystems.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/ResourceTraits.hpp"

using rtype::core::asset::AssetId;
using rtype::ecs::Entity;
using rtype::plugins::backend::graphics::ParticleSystemDesc;
using rtype::plugins::backend::graphics::ParticleSystemId;
using rtype::plugins::particle::CpuSimulation;
using rtype::plugins::particle::EmitterState;
using rtype::plugins::particle::ParticleSystems;

static_assert(rtype::ecs::Resource<ParticleSystems>);

namespace {
/// @return The state of an emitter of the effect "sparks" simulated on the GPU, by the system @p system.
EmitterState makeGpuState(ParticleSystemId system) {
    return EmitterState{.effect = AssetId::fromPath("sparks"), .seed = 3, .simulation = system};
}
}  // namespace

TEST(ParticleSystemsTests, StartsEmpty) {
    const ParticleSystems systems;
    EXPECT_EQ(systems.getCount(), 0U);
    EXPECT_EQ(systems.find(Entity{1, 1}), nullptr);
}

TEST(ParticleSystemsTests, StartStoresTheStateByEntity) {
    ParticleSystems systems;
    const Entity entity{4, 1};
    EmitterState& stored = systems.start(entity, makeGpuState({.index = 2, .generation = 1}));

    EXPECT_EQ(&stored, systems.find(entity));
    EXPECT_EQ(systems.getCount(), 1U);
    EXPECT_EQ(stored.seed, 3U);
    ASSERT_TRUE(std::holds_alternative<ParticleSystemId>(stored.simulation));
    EXPECT_EQ(std::get<ParticleSystemId>(stored.simulation), (ParticleSystemId{.index = 2, .generation = 1}));
    EXPECT_EQ(systems.find(Entity{4, 2}), nullptr);
}

TEST(ParticleSystemsTests, StartKeepsACpuSimulation) {
    ParticleSystems systems;
    const EmitterState& stored = systems.start(
        Entity{1, 1},
        EmitterState{.effect = AssetId::fromPath("smoke"), .simulation = CpuSimulation{ParticleSystemDesc{}}});

    ASSERT_TRUE(std::holds_alternative<CpuSimulation>(stored.simulation));
    EXPECT_EQ(std::get<CpuSimulation>(stored.simulation).getCapacity(), 1024U);
}

TEST(ParticleSystemsTests, StartingAPlayingEmitterThrows) {
    ParticleSystems systems;
    const Entity entity{1, 1};
    systems.start(entity, makeGpuState({.index = 0, .generation = 1}));

    EXPECT_THROW(systems.start(entity, makeGpuState({.index = 1, .generation = 1})), std::logic_error);
    EXPECT_EQ(std::get<ParticleSystemId>(systems.find(entity)->simulation).index, 0U);
}

TEST(ParticleSystemsTests, StopGivesBackTheStateToDestroy) {
    ParticleSystems systems;
    const Entity entity{1, 1};
    systems.start(entity, makeGpuState({.index = 5, .generation = 2}));

    const std::optional<EmitterState> stopped = systems.stop(entity);
    ASSERT_TRUE(stopped);
    EXPECT_EQ(std::get<ParticleSystemId>(stopped->simulation), (ParticleSystemId{.index = 5, .generation = 2}));
    EXPECT_EQ(systems.find(entity), nullptr);
    EXPECT_EQ(systems.getCount(), 0U);
    EXPECT_FALSE(systems.stop(entity));
}

TEST(ParticleSystemsTests, StopThenStartRestarts) {
    ParticleSystems systems;
    const Entity entity{1, 1};
    systems.start(entity, makeGpuState({.index = 0, .generation = 1}));
    systems.find(entity)->time = 2.0F;
    static_cast<void>(systems.stop(entity));
    systems.start(entity, makeGpuState({.index = 1, .generation = 1}));

    EXPECT_FLOAT_EQ(systems.find(entity)->time, 0.0F);
}

TEST(ParticleSystemsTests, GetEmittersListsEveryPlayingEmitter) {
    ParticleSystems systems;
    systems.start(Entity{1, 1}, makeGpuState({.index = 0, .generation = 1}));
    systems.start(Entity{2, 1}, makeGpuState({.index = 1, .generation = 1}));

    EXPECT_EQ(systems.getEmitters().size(), 2U);
    EXPECT_TRUE(systems.getEmitters().contains(Entity{1, 1}));
    EXPECT_TRUE(systems.getEmitters().contains(Entity{2, 1}));
}

TEST(ParticleSystemsTests, PickSeedGivesADifferentSeedEachTime) {
    ParticleSystems systems;
    const std::uint32_t first = systems.pickSeed();
    const std::uint32_t second = systems.pickSeed();
    EXPECT_NE(first, second);
    EXPECT_NE(first, 0U);
}

TEST(ParticleSystemsTests, AFallbackIsReportedOncePerEffect) {
    ParticleSystems systems;
    EXPECT_TRUE(systems.markFallbackReported(AssetId::fromPath("sparks")));
    EXPECT_FALSE(systems.markFallbackReported(AssetId::fromPath("sparks")));
    EXPECT_TRUE(systems.markFallbackReported(AssetId::fromPath("smoke")));
}
