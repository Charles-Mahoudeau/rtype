/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** EditorPlaySessionTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>

#include "editor/play/PlaySession.hpp"
#include "editor/scene/SceneEdit.hpp"
#include "plugins/capsule/Capsule.hpp"
#include "plugins/capsule/CapsulePlugin.hpp"
#include "plugins/capsule/Capsules.hpp"
#include "plugins/capsule/LoadedCapsule.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/resource/Resources.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/Schedules.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/world/World.hpp"

namespace rtype::test::play_session {
struct Health {
    std::int32_t value{0};
};
RTYPE_ECS_COMPONENT(Health, "test.playSession.Health", value);

/// @brief How many times Update ran.
struct Updates {
    int count{0};
};
RTYPE_ECS_RESOURCE(Updates, "test.playSession.Updates", count);
}  // namespace rtype::test::play_session

namespace {
namespace capsule = rtype::plugins::capsule;
using rtype::editor::play::PlaySession;
using rtype::test::play_session::Health;
using rtype::test::play_session::Updates;

void countUpdates(rtype::ecs::ResMut<Updates> updates) { ++updates->count; }

/// @brief An App with the capsule plugin, a scene of one node with a Health of 10, shown, and a system counting Update.
class PlaySessionTest : public ::testing::Test {
  protected:
    void SetUp() override {
        static_cast<void>(app.getWorld().getComponents().registerType<Health>());
        app.insertResource(Updates{});
        app.addSystem(rtype::ecs::Phase::kUpdate, "test.countUpdates", &countUpdates);
        app.addPlugin(capsule::CapsulePlugin{});
        app.finishPlugins();
        capsule::Capsule level{"scenes/level.scene"};
        level.editNode(0).values.set(app.getWorld().getComponents(), Health{.value = 10});
        app.getWorld().getResources().getMut<capsule::Capsules>()->define(std::move(level));
        ASSERT_EQ(rtype::editor::scene::open(app.getWorld(), sceneId), std::nullopt);
        root = app.getWorld().getResources().get<capsule::LoadedCapsule>()->root;
        PlaySession::edit(app);
    }

    [[nodiscard]] int updates() { return app.getWorld().getResources().get<Updates>()->count; }

    const capsule::CapsuleId sceneId = capsule::CapsuleId::fromPath("scenes/level.scene");
    rtype::ecs::App app;
    rtype::ecs::Entity root;
    PlaySession session;
};
}  // namespace

TEST_F(PlaySessionTest, EditingKeepsTheGameplayStill) {
    app.runFrame();
    EXPECT_EQ(updates(), 0);
    EXPECT_FALSE(session.isPlaying());
}

TEST_F(PlaySessionTest, PlayRunsTheGameplay) {
    session.play(app);
    EXPECT_TRUE(session.isPlaying());
    app.runFrame();
    EXPECT_EQ(updates(), 1);
}

TEST_F(PlaySessionTest, PauseAndStepWhilePlaying) {
    session.play(app);
    session.togglePause(app);
    app.runFrame();
    EXPECT_EQ(updates(), 0);
    session.step(app);
    app.runFrame();
    app.runFrame();
    EXPECT_EQ(updates(), 1);
    session.togglePause(app);
    app.runFrame();
    EXPECT_EQ(updates(), 2);
}

TEST_F(PlaySessionTest, PauseAndStepDoNothingWhileEditing) {
    session.togglePause(app);
    session.step(app);
    app.runFrame();
    EXPECT_EQ(updates(), 0);
    EXPECT_TRUE(app.getSchedules().isPaused());
}

TEST_F(PlaySessionTest, StopDespawnsWhatPlayingSpawned) {
    session.play(app);
    const rtype::ecs::Entity bullet = app.getWorld().spawn(Health{.value = 1});
    ASSERT_EQ(session.stop(app, sceneId), std::nullopt);
    EXPECT_FALSE(app.getWorld().isAlive(bullet));
    EXPECT_TRUE(app.getSchedules().isPaused());
    EXPECT_FALSE(session.isPlaying());
}

TEST_F(PlaySessionTest, StopPutsTheSceneBackAsItsCapsuleSays) {
    session.play(app);
    app.getWorld().getComponents().getMut<Health>(root)->value = 3;  // what the game did while playing
    ASSERT_EQ(session.stop(app, sceneId), std::nullopt);
    app.runFrame();  // the capsule plugin respawns it
    ASSERT_TRUE(app.getWorld().isAlive(root));
    EXPECT_EQ(app.getWorld().getComponents().get<Health>(root)->value, 10);
}

TEST_F(PlaySessionTest, StopShowsTheSceneAgainIfTheGameDespawnedIt) {
    session.play(app);
    static_cast<void>(app.getWorld().despawn(root));
    ASSERT_EQ(session.stop(app, sceneId), std::nullopt);
    const rtype::ecs::Entity shown = app.getWorld().getResources().get<capsule::LoadedCapsule>()->root;
    ASSERT_TRUE(app.getWorld().isAlive(shown));
    EXPECT_EQ(app.getWorld().getComponents().get<Health>(shown)->value, 10);
}

TEST_F(PlaySessionTest, StopKeepsWhatWasThereBeforePlay) {
    const rtype::ecs::Entity manager = app.getWorld().spawn(Health{.value = 99});
    session.play(app);
    ASSERT_EQ(session.stop(app, std::nullopt), std::nullopt);
    EXPECT_TRUE(app.getWorld().isAlive(manager));
}

TEST_F(PlaySessionTest, StopDoesNothingWhileEditing) {
    const rtype::ecs::Entity entity = app.getWorld().spawn(Health{});
    EXPECT_EQ(session.stop(app, sceneId), std::nullopt);
    EXPECT_TRUE(app.getWorld().isAlive(entity));
}
