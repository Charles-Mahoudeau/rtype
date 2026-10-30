/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ParticlePluginTests
*/

#include <gtest/gtest.h>

#include <chrono>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <span>
#include <utility>
#include <variant>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Particle.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/particle/CpuSimulation.hpp"
#include "plugins/particle/ParticleEffect.hpp"
#include "plugins/particle/ParticleEffectAssets.hpp"
#include "plugins/particle/ParticleEmitter.hpp"
#include "plugins/particle/ParticleEvents.hpp"
#include "plugins/particle/ParticlePlugin.hpp"
#include "plugins/particle/ParticleSystems.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/ecs/world/World.hpp"

using rtype::core::asset::AssetId;
using rtype::plugins::backend::graphics::ParticleDraw;
using rtype::plugins::backend::graphics::ParticleInstance;
using rtype::plugins::backend::graphics::ParticleSimulation;
using rtype::plugins::backend::graphics::ParticleSystemId;
using rtype::plugins::particle::ParticleEffect;
using rtype::plugins::particle::ParticleEffectAssets;
using rtype::plugins::particle::ParticleEmitter;
using rtype::plugins::particle::ParticleOneShot;
using rtype::plugins::particle::ParticlesFinished;
using rtype::plugins::particle::ParticleSystems;
using rtype::plugins::particle::SpawnParticles;
using rtype::plugins::transform::Transform;

namespace {
/// @brief An App with the BackendPlugin over the fake backend, 0.1 second per frame, the texture spark.png loaded and
/// the effects "burst" (one burst of 8 particles living 0.25 seconds, once) and "stream" (10 particles per second,
/// looping).
class ParticleApp {
  public:
    explicit ParticleApp(bool gpuParticles = false)
        : _state{std::make_shared<rtype::test::plugins::FakeBackendState>()}, _app{rtype::ecs::RunMode::kClient} {
        _state->gpuParticles = gpuParticles;
        _app.addPlugin(rtype::plugins::backend::BackendPlugin{rtype::test::plugins::makeFakeRegistry(_state),
                                                              rtype::test::plugins::makeFakeSettings()});
        _app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::milliseconds{100}));
        _texture = world().getResources().getMut<rtype::plugins::backend::TextureAssets>()->load(
            "spark.png", rtype::test::plugins::twoPixelPng());

        ParticleEffect burst;
        burst.texture = _texture;
        burst.desc.loop = false;
        burst.desc.duration = 0.05F;
        burst.desc.lifetime = {.min = 0.25F, .max = 0.25F};
        burst.desc.bursts.push_back({.count = 8});
        _burst = effects().add("burst", std::move(burst));

        ParticleEffect stream;
        stream.texture = _texture;
        stream.desc.rate = 10.0F;
        _stream = effects().add("stream", std::move(stream));
    }
    ~ParticleApp() = default;
    ParticleApp(const ParticleApp& other) = delete;
    ParticleApp& operator=(const ParticleApp& other) = delete;
    ParticleApp(ParticleApp&& other) = delete;
    ParticleApp& operator=(ParticleApp&& other) = delete;

    [[nodiscard]] rtype::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] rtype::ecs::World& world() noexcept { return _app.getWorld(); }
    [[nodiscard]] rtype::test::plugins::FakeBackendState& state() noexcept { return *_state; }
    [[nodiscard]] ParticleEffectAssets& effects() { return *world().getResources().getMut<ParticleEffectAssets>(); }
    [[nodiscard]] ParticleSystems& systems() { return *world().getResources().getMut<ParticleSystems>(); }
    [[nodiscard]] AssetId burst() const noexcept { return _burst; }
    [[nodiscard]] AssetId stream() const noexcept { return _stream; }

    /// @brief Spawns an entity at @p position playing @p emitter.
    rtype::ecs::Entity spawnEmitter(ParticleEmitter emitter, glm::vec2 position = {0.0F, 0.0F}) {
        const rtype::ecs::Entity entity = world().spawn();
        world().getComponents().add(entity, Transform{.position = position});
        world().getComponents().add(entity, emitter);
        return entity;
    }

    /// @return The particle draws of one more frame.
    std::vector<ParticleDraw> drawFrame() {
        _state->particleDraws.clear();
        _state->drawOrder.clear();
        _app.runFrame();
        return _state->particleDraws;
    }

  private:
    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    rtype::ecs::App _app;                                            ///< The App under test.
    AssetId _texture;                                                ///< spark.png.
    AssetId _burst;                                                  ///< The effect "burst".
    AssetId _stream;                                                 ///< The effect "stream".
};

/// @return The number of CPU instances @p draw hands over (0 for a GPU system).
std::size_t countInstances(const ParticleDraw& draw) {
    const auto* instances = std::get_if<std::span<const ParticleInstance>>(&draw.particles);
    return instances != nullptr ? instances->size() : 0;
}
}  // namespace

TEST(ParticlePlugin, ComesWithTheBackendPlugin) {
    ParticleApp app;
    EXPECT_TRUE(app.app().hasPlugin(rtype::plugins::particle::ParticlePlugin::kName));
    EXPECT_TRUE(app.world().getResources().has<ParticleEffectAssets>());
    EXPECT_TRUE(app.world().getResources().has<ParticleSystems>());
    EXPECT_TRUE(app.world().getComponents().find("rtype.ParticleEmitter"));
    EXPECT_TRUE(app.world().getComponents().find("rtype.ParticleOneShot"));
}

TEST(ParticlePlugin, DrawsACpuEmitterAtItsLayer) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.stream(), .layer = 3}));
    static_cast<void>(app.drawFrame());  // GlobalTransform is added at the end of this frame
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(countInstances(draws.at(0)), 1U);
    EXPECT_EQ(app.state().drawOrder, (std::vector<char>{'p'}));
    EXPECT_TRUE(app.state().createdParticleSystems.empty());
}

TEST(ParticlePlugin, DrawsWithTheLookOfTheEffect) {
    ParticleApp app;
    ParticleEffect sparks = *app.effects().get(app.stream());
    sparks.blend = rtype::plugins::backend::graphics::BlendMode::kAlpha;
    sparks.alignment = rtype::plugins::backend::graphics::ParticleAlignment::kVelocity;
    sparks.stretch = 0.1F;
    const AssetId id = app.effects().add("sparks", std::move(sparks));
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = id}));
    static_cast<void>(app.drawFrame());
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(draws.at(0).blend, rtype::plugins::backend::graphics::BlendMode::kAlpha);
    EXPECT_EQ(draws.at(0).alignment, rtype::plugins::backend::graphics::ParticleAlignment::kVelocity);
    EXPECT_FLOAT_EQ(draws.at(0).stretch, 0.1F);
}

TEST(ParticlePlugin, EmitsTheRateOverTheFrames) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.stream()}));
    static_cast<void>(app.drawFrame());
    for (int i = 0; i < 4; ++i) {
        static_cast<void>(app.drawFrame());
    }
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(countInstances(draws.at(0)), 5U);
}

TEST(ParticlePlugin, AStoppedEmitterEmitsNothing) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = app.stream(), .playing = false}));
    static_cast<void>(app.drawFrame());
    EXPECT_TRUE(app.drawFrame().empty());
    EXPECT_EQ(app.systems().getCount(), 1U);
}

TEST(ParticlePlugin, AnUnknownEffectPlaysNothing) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(ParticleEmitter{.effect = AssetId::fromPath("missing")}));
    static_cast<void>(app.drawFrame());
    EXPECT_TRUE(app.drawFrame().empty());
    EXPECT_EQ(app.systems().getCount(), 0U);
}

TEST(ParticlePlugin, RemovingTheEmitterStopsIt) {
    ParticleApp app;
    const rtype::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.stream()});
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    EXPECT_EQ(app.systems().getCount(), 1U);
    app.world().getComponents().remove<ParticleEmitter>(entity);
    EXPECT_TRUE(app.drawFrame().empty());
    EXPECT_EQ(app.systems().getCount(), 0U);
}

TEST(ParticlePlugin, ChangingTheEffectRestartsTheEmitter) {
    ParticleApp app;
    const rtype::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.stream()});
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    app.world().getComponents().getMut<ParticleEmitter>(entity)->effect = app.burst();
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(countInstances(draws.at(0)), 8U);
    EXPECT_EQ(app.systems().find(entity)->effect, app.burst());
}

TEST(ParticlePlugin, AOneShotEmitterFinishesAndIsDespawned) {
    ParticleApp app;
    const rtype::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.burst()});
    app.world().getComponents().add(entity, ParticleOneShot{});
    rtype::ecs::EventCursor cursor;
    static_cast<void>(app.drawFrame());
    EXPECT_EQ(countInstances(app.drawFrame().at(0)), 8U);
    int sent = 0;
    for (int i = 0; i < 3; ++i) {
        static_cast<void>(app.drawFrame());
        for (const ParticlesFinished& finished : app.world().getEvents().read<ParticlesFinished>(cursor)) {
            sent += finished.entity == entity ? 1 : 0;
        }
    }
    EXPECT_EQ(sent, 1);
    EXPECT_FALSE(app.world().isAlive(entity));
    static_cast<void>(app.drawFrame());
    EXPECT_EQ(app.systems().getCount(), 0U);
}

TEST(ParticlePlugin, SpawnParticlesPlaysAOneShotEffect) {
    ParticleApp app;
    app.world().getEvents().send(SpawnParticles{.effect = app.burst(), .position = {40.0F, 20.0F}, .layer = 1});
    static_cast<void>(app.drawFrame());
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    ASSERT_EQ(countInstances(draws.at(0)), 8U);
    EXPECT_EQ(std::get<std::span<const ParticleInstance>>(draws.at(0).particles)[0].position,
              (glm::vec2{40.0F, 20.0F}));
    for (int i = 0; i < 4; ++i) {
        static_cast<void>(app.drawFrame());
    }
    EXPECT_EQ(app.systems().getCount(), 0U);
}

TEST(ParticlePlugin, RunsOnTheGpuWhenTheRendererCan) {
    ParticleApp app{true};
    const rtype::ecs::Entity entity = app.spawnEmitter(ParticleEmitter{.effect = app.stream()}, {5.0F, 6.0F});
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    const auto draws = app.drawFrame();
    ASSERT_EQ(app.state().createdParticleSystems.size(), 1U);
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_TRUE(std::holds_alternative<ParticleSystemId>(draws.at(0).particles));
    ASSERT_EQ(app.state().simulations.size(), 2U);
    EXPECT_EQ(app.state().simulations.at(1).position, (glm::vec2{5.0F, 6.0F}));
    EXPECT_EQ(app.state().simulations.at(1).firstParticle, app.state().simulations.at(0).spawnCount);

    app.world().despawn(entity);
    static_cast<void>(app.drawFrame());
    ASSERT_EQ(app.state().destroyedParticleSystems.size(), 1U);
    EXPECT_EQ(app.state().destroyedParticleSystems.at(0), std::get<ParticleSystemId>(draws.at(0).particles));
}

TEST(ParticlePlugin, FallsBackToTheCpuWhenTheGpuIsAskedButMissing) {
    ParticleApp app;
    static_cast<void>(app.spawnEmitter(
        ParticleEmitter{.effect = app.stream(), .overrideSimulation = true, .simulation = ParticleSimulation::kGpu}));
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    const auto draws = app.drawFrame();
    ASSERT_EQ(draws.size(), 1U);
    EXPECT_EQ(countInstances(draws.at(0)), 2U);
}

TEST(ParticlePlugin, TheEmitterCanForceTheCpu) {
    ParticleApp app{true};
    static_cast<void>(app.spawnEmitter(
        ParticleEmitter{.effect = app.stream(), .overrideSimulation = true, .simulation = ParticleSimulation::kCpu}));
    static_cast<void>(app.drawFrame());
    static_cast<void>(app.drawFrame());
    EXPECT_TRUE(app.state().createdParticleSystems.empty());
    EXPECT_EQ(countInstances(app.drawFrame().at(0)), 2U);
}
