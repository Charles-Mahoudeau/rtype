/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** BackendPluginTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_uint2.hpp>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/WindowEvents.hpp"
#include "plugins/backend/event/Event.hpp"
#include "plugins/backend/event/PlatformEvent.hpp"
#include "plugins/backend/graphics/AssetId.hpp"
#include "plugins/backend/graphics/Camera.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/input/Control.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputEvents.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
/// @brief An App with the BackendPlugin over the fake platform and renderer.
class BackendApp {
  public:
    explicit BackendApp(rtype::ecs::RunMode mode = rtype::ecs::RunMode::kClient)
        : _state{std::make_shared<rtype::test::plugins::FakeBackendState>()}, _app{mode} {
        _app.addPlugin(rtype::plugins::backend::BackendPlugin{rtype::test::plugins::makeFakeRegistry(_state),
                                                              rtype::test::plugins::makeFakeSettings()});
    }
    ~BackendApp() = default;
    BackendApp(const BackendApp& other) = delete;
    BackendApp& operator=(const BackendApp& other) = delete;
    BackendApp(BackendApp&& other) = delete;
    BackendApp& operator=(BackendApp&& other) = delete;

    [[nodiscard]] rtype::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] rtype::test::plugins::FakeBackendState& state() noexcept { return *_state; }

  private:
    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    rtype::ecs::App _app;                                            ///< The App under test.
};
}  // namespace

TEST(BackendPlugin, CreatesNothingBeforeTheFirstFrame) {
    BackendApp app;
    EXPECT_EQ(app.state().initCount, 0);
    EXPECT_TRUE(app.app().getWorld().getResources().has<rtype::plugins::backend::Backend>());
    EXPECT_EQ(app.app().getWorld().getResources().get<rtype::plugins::backend::Backend>()->platform, nullptr);
}

TEST(BackendPlugin, InsertsTheTextureAssetsAndRegistersSpriteRenderer) {
    BackendApp app;
    EXPECT_TRUE(app.app().getWorld().getResources().has<rtype::plugins::backend::TextureAssets>());
    EXPECT_TRUE(app.app().getWorld().getComponents().find("rtype.SpriteRenderer"));
    EXPECT_TRUE(app.app().getWorld().getComponents().find("rtype.Camera"));
}

TEST(BackendPlugin, LoadsTheQueuedTexturesBeforeTheFrame) {
    BackendApp app;
    const auto id = app.app().getWorld().getResources().getMut<rtype::plugins::backend::TextureAssets>()->load(
        "two.png", rtype::test::plugins::twoPixelPng());
    const auto& textures = app.app().getWorld().getResources().require<rtype::plugins::backend::TextureAssets>();
    app.app().runFrame();
    EXPECT_EQ(app.state().createdTextures.size(), 1U);
    EXPECT_TRUE(textures.getTexture(id));
}

namespace {
/// @brief Spawns an entity drawn with @p texture at @p position on @p layer.
rtype::ecs::Entity spawnSprite(BackendApp& app, rtype::plugins::backend::AssetId texture, glm::vec2 position,
                               std::int32_t layer) {
    rtype::ecs::World& world = app.app().getWorld();
    const rtype::ecs::Entity entity = world.spawn();
    world.getComponents().add(entity, rtype::plugins::transform::Transform{.position = position});
    world.getComponents().add(entity, rtype::plugins::backend::SpriteRenderer{.texture = texture, .layer = layer});
    return entity;
}

/// @return The id of a texture asked for under @p name, loaded at the next frame.
rtype::plugins::backend::AssetId loadTexture(BackendApp& app, const std::string& name) {
    return app.app().getWorld().getResources().getMut<rtype::plugins::backend::TextureAssets>()->load(
        name, rtype::test::plugins::twoPixelPng());
}
}  // namespace

TEST(BackendPlugin, BringsTheTransformPlugin) {
    BackendApp app;
    EXPECT_TRUE(app.app().hasPlugin(rtype::plugins::transform::TransformPlugin::kName));
}

TEST(BackendPlugin, DrawsTheSpritesAtTheirGlobalTransform) {
    BackendApp app;
    spawnSprite(app, loadTexture(app, "ship.png"), {10.0F, 20.0F}, 0);
    app.app().runFrame();
    ASSERT_EQ(app.state().sprites.size(), 1U);
    EXPECT_EQ(app.state().sprites[0].position, (glm::vec2{10.0F, 20.0F}));
}

TEST(BackendPlugin, DrawsTheSpritesByLayer) {
    BackendApp app;
    const auto texture = loadTexture(app, "ship.png");
    spawnSprite(app, texture, {3.0F, 0.0F}, 5);
    spawnSprite(app, texture, {1.0F, 0.0F}, -2);
    spawnSprite(app, texture, {2.0F, 0.0F}, 0);
    app.app().runFrame();
    ASSERT_EQ(app.state().sprites.size(), 3U);
    EXPECT_FLOAT_EQ(app.state().sprites[0].position.x, 1.0F);
    EXPECT_FLOAT_EQ(app.state().sprites[1].position.x, 2.0F);
    EXPECT_FLOAT_EQ(app.state().sprites[2].position.x, 3.0F);
}

TEST(BackendPlugin, GroupsTheSpritesOfALayerByTexture) {
    BackendApp app;
    const auto first = loadTexture(app, "first.png");
    const auto second = loadTexture(app, "second.png");
    spawnSprite(app, first, {1.0F, 0.0F}, 0);
    spawnSprite(app, second, {2.0F, 0.0F}, 0);
    spawnSprite(app, first, {3.0F, 0.0F}, 0);
    app.app().runFrame();
    ASSERT_EQ(app.state().sprites.size(), 3U);
    EXPECT_EQ(app.state().sprites[0].texture, app.state().sprites[1].texture);
    EXPECT_FLOAT_EQ(app.state().sprites[0].position.x, 1.0F);
    EXPECT_FLOAT_EQ(app.state().sprites[1].position.x, 3.0F);
    EXPECT_NE(app.state().sprites[2].texture, app.state().sprites[0].texture);
}

TEST(BackendPlugin, SkipsSpritesWhoseTextureIsNotLoaded) {
    BackendApp app;
    spawnSprite(app, rtype::plugins::backend::AssetId::fromPath("never-loaded.png"), {0.0F, 0.0F}, 0);
    app.app().runFrame();
    EXPECT_TRUE(app.state().sprites.empty());
}

TEST(BackendPlugin, LoadsNoTextureOnAServer) {
    BackendApp app{rtype::ecs::RunMode::kServer};
    const auto id = app.app().getWorld().getResources().getMut<rtype::plugins::backend::TextureAssets>()->load(
        "two.png", rtype::test::plugins::twoPixelPng());
    const auto& textures = app.app().getWorld().getResources().require<rtype::plugins::backend::TextureAssets>();
    app.app().runFrame();
    EXPECT_TRUE(app.state().createdTextures.empty());
    EXPECT_EQ(textures.get(id)->state, rtype::plugins::backend::AssetState::kQueued);
}

TEST(BackendPlugin, CreatesTheBackendAtStartup) {
    BackendApp app;
    app.app().runFrame();
    EXPECT_EQ(app.state().initCount, 1);
    const auto* backend = app.app().getWorld().getResources().get<rtype::plugins::backend::Backend>();
    ASSERT_NE(backend, nullptr);
    EXPECT_NE(backend->platform, nullptr);
    EXPECT_NE(backend->renderer, nullptr);
    EXPECT_EQ(backend->platform->getTitle(), "Test");
}

TEST(BackendPlugin, FramesEveryFrameWithTheClearColor) {
    BackendApp app;
    app.app().getWorld().getResources().insert(rtype::plugins::backend::ClearColor{.value = {1.0F, 0.0F, 0.0F, 1.0F}});
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.state().beginFrames, 2);
    EXPECT_EQ(app.state().endFrames, 2);
    EXPECT_EQ(app.state().clearColor, (rtype::plugins::backend::graphics::Color{1.0F, 0.0F, 0.0F, 1.0F}));
}

TEST(BackendPlugin, SendsThePlatformEventsAsEcsEvents) {
    BackendApp app;
    app.state().frames.push_back(
        {rtype::plugins::backend::event::KeyPressed{.key = rtype::plugins::backend::input::Key::kA},
         rtype::plugins::backend::event::TextEntered{.codepoint = U'a'}});
    app.app().runFrame();
    EXPECT_EQ(app.app().getWorld().getEvents().getCount<rtype::plugins::backend::event::KeyPressed>(), 1U);
    EXPECT_EQ(app.app().getWorld().getEvents().getCount<rtype::plugins::backend::event::TextEntered>(), 1U);
}

TEST(BackendPlugin, ResizesTheRenderer) {
    BackendApp app;
    app.state().frames.push_back({rtype::plugins::backend::event::Resized{.width = 1024, .height = 768}});
    app.app().runFrame();
    ASSERT_EQ(app.state().resizes.size(), 1U);
    EXPECT_EQ(app.state().resizes[0], glm::uvec2(1024, 768));
}

TEST(BackendPlugin, WaitsAndDrawsNothingWhileMinimized) {
    BackendApp app;
    app.state().framebufferSize = {0, 0};
    app.app().runFrame();
    EXPECT_EQ(app.state().waits, 1);
    EXPECT_EQ(app.state().polls, 0);
    EXPECT_EQ(app.state().beginFrames, 0);
    EXPECT_EQ(app.state().endFrames, 0);
}

TEST(BackendPlugin, TheAppStopsWhenTheWindowCloses) {
    BackendApp app;
    app.state().closeAfterPolls = 3;
    EXPECT_EQ(app.app().run(), 0);
    EXPECT_EQ(app.state().polls, 3);
}

TEST(BackendPlugin, DoesNotRunOnADedicatedServer) {
    BackendApp app{rtype::ecs::RunMode::kServer};
    app.app().runFrame();
    EXPECT_EQ(app.state().initCount, 0);
    EXPECT_EQ(app.state().polls, 0);
}

TEST(BackendPlugin, FeedsInputWhicheverPluginIsAddedFirst) {
    for (const bool inputFirst : {true, false}) {
        auto state = std::make_shared<rtype::test::plugins::FakeBackendState>();
        rtype::ecs::App app{rtype::ecs::RunMode::kClient};
        rtype::plugins::backend::BackendPlugin backend{rtype::test::plugins::makeFakeRegistry(state),
                                                       rtype::test::plugins::makeFakeSettings()};
        if (inputFirst) {
            app.addPlugin(rtype::plugins::input::InputPlugin{}).addPlugin(std::move(backend));
        } else {
            app.addPlugin(std::move(backend)).addPlugin(rtype::plugins::input::InputPlugin{});
        }
        state->frames.push_back(
            {rtype::plugins::backend::event::KeyPressed{.key = rtype::plugins::backend::input::Key::kSpace}});
        app.runFrame();
        const auto& input = app.getWorld().getResources().require<rtype::plugins::input::Input>();
        EXPECT_GT(input.read(rtype::plugins::input::Control::key(rtype::plugins::backend::input::Key::kSpace)), 0.0F)
            << (inputFirst ? "InputPlugin added first" : "BackendPlugin added first");
    }
}

TEST(BackendPlugin, ShowsNoCameraWhenNoEntityHasOne) {
    BackendApp app;
    app.app().runFrame();
    EXPECT_TRUE(app.state().cameras.empty());
}

TEST(BackendPlugin, ShowsTheCameraWithTheHighestPriority) {
    BackendApp app;
    rtype::ecs::World& world = app.app().getWorld();
    world.spawn(rtype::plugins::backend::graphics::Camera{.position = {1.0F, 0.0F, 0.0F}, .priority = 1});
    world.spawn(rtype::plugins::backend::graphics::Camera{.position = {2.0F, 0.0F, 0.0F}, .priority = 5});
    world.spawn(rtype::plugins::backend::graphics::Camera{.position = {3.0F, 0.0F, 0.0F}, .priority = -2});
    app.app().runFrame();
    ASSERT_EQ(app.state().cameras.size(), 1U);
    EXPECT_FLOAT_EQ(app.state().cameras[0].position.x, 2.0F);
}

TEST(BackendPlugin, AppliesTheCameraEveryFrameItIsDrawn) {
    BackendApp app;
    app.app().getWorld().spawn(rtype::plugins::backend::graphics::Camera{});
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.state().cameras.size(), 2U);
    app.state().framebufferSize = {0, 0};
    app.app().runFrame();
    EXPECT_EQ(app.state().cameras.size(), 2U);
}

TEST(BackendPlugin, BringsTheInputPlugin) {
    BackendApp app;
    EXPECT_TRUE(app.app().hasPlugin(rtype::plugins::input::InputPlugin::kName));
    EXPECT_TRUE(app.app().getWorld().getResources().has<rtype::plugins::input::Input>());
}

TEST(BackendPlugin, SendsThePlatformEventsInOrder) {
    BackendApp app;
    app.state().frames.push_back(
        {rtype::plugins::backend::event::KeyReleased{.key = rtype::plugins::backend::input::Key::kA},
         rtype::plugins::backend::event::KeyPressed{.key = rtype::plugins::backend::input::Key::kA}});
    app.app().runFrame();
    rtype::ecs::EventCursor cursor;
    const auto events = app.app().getWorld().getEvents().read<rtype::plugins::backend::event::PlatformEvent>(cursor);
    ASSERT_EQ(events.size(), 2U);
    auto it = events.begin();
    EXPECT_TRUE(std::holds_alternative<rtype::plugins::backend::event::KeyReleased>(it->event));
    ++it;
    EXPECT_TRUE(std::holds_alternative<rtype::plugins::backend::event::KeyPressed>(it->event));
}
