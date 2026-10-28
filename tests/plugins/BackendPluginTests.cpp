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
#include "core/asset/AssetId.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/BackendRegistry.hpp"
#include "plugins/backend/BackendResources.hpp"
#include "plugins/backend/WindowEvents.hpp"
#include "plugins/backend/event/Event.hpp"
#include "plugins/backend/event/PlatformEvent.hpp"
#include "plugins/backend/graphics/BlendMode.hpp"
#include "plugins/backend/graphics/Camera.hpp"
#include "plugins/backend/graphics/Sprite.hpp"
#include "plugins/backend/graphics/SpriteQueue.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/Text.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/graphics/View.hpp"
#include "plugins/backend/graphics/VisibleArea.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/input/Control.hpp"
#include "plugins/input/Input.hpp"
#include "plugins/input/InputEvents.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/Field.hpp"
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
rtype::ecs::Entity spawnSprite(BackendApp& app, rtype::core::asset::AssetId texture, glm::vec2 position,
                               std::int32_t layer) {
    rtype::ecs::World& world = app.app().getWorld();
    const rtype::ecs::Entity entity = world.spawn();
    world.getComponents().add(entity, rtype::plugins::transform::Transform{.position = position});
    world.getComponents().add(entity, rtype::plugins::backend::SpriteRenderer{.texture = texture, .layer = layer});
    return entity;
}

/// @return The id of a texture asked for under @p name, loaded at the next frame.
rtype::core::asset::AssetId loadTexture(BackendApp& app, const std::string& name) {
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

TEST(BackendPlugin, DrawsTheSpritesWithTheirBlendMode) {
    BackendApp app;
    const rtype::ecs::Entity entity = spawnSprite(app, loadTexture(app, "laser.png"), {0.0F, 0.0F}, 0);
    app.app().getWorld().getComponents().getMut<rtype::plugins::backend::SpriteRenderer>(entity)->blend =
        rtype::plugins::backend::graphics::BlendMode::kAdditive;
    app.app().runFrame();
    ASSERT_EQ(app.state().sprites.size(), 1U);
    EXPECT_EQ(app.state().sprites[0].blend, rtype::plugins::backend::graphics::BlendMode::kAdditive);
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
    spawnSprite(app, rtype::core::asset::AssetId::fromPath("never-loaded.png"), {0.0F, 0.0F}, 0);
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

TEST(BackendPlugin, ReflectsTheCameraPriority) {
    using Traits = rtype::ecs::ComponentTraits<rtype::plugins::backend::graphics::Camera>;
    EXPECT_EQ(Traits::kName, "rtype.Camera");
    ASSERT_EQ(Traits::kFields.size(), 1U);
    EXPECT_EQ(Traits::kFields[0].name, "priority");
    EXPECT_EQ(Traits::kFields[0].type, rtype::ecs::FieldType::kI32);
}

TEST(BackendPlugin, ShowsNoCameraWhenNoEntityHasOne) {
    BackendApp app;
    app.app().runFrame();
    EXPECT_TRUE(app.state().views.empty());
}

TEST(BackendPlugin, ShowsTheCameraWithTheHighestPriority) {
    BackendApp app;
    rtype::ecs::World& world = app.app().getWorld();
    world.spawn(rtype::plugins::transform::Transform{.position = {1.0F, 0.0F}},
                rtype::plugins::backend::graphics::Camera{.priority = 1});
    world.spawn(rtype::plugins::transform::Transform{.position = {2.0F, 0.0F}},
                rtype::plugins::backend::graphics::Camera{.priority = 5});
    world.spawn(rtype::plugins::transform::Transform{.position = {3.0F, 0.0F}},
                rtype::plugins::backend::graphics::Camera{.priority = -2});
    app.app().runFrame();
    ASSERT_EQ(app.state().views.size(), 1U);
    EXPECT_FLOAT_EQ(app.state().views[0].position.x, 2.0F);
}

TEST(BackendPlugin, ShowsTheCameraFromItsTransform) {
    BackendApp app;
    app.app().getWorld().spawn(rtype::plugins::transform::Transform{.position = {40.0F, -12.0F}, .rotation = 0.5F},
                               rtype::plugins::backend::graphics::Camera{
                                   .projection = rtype::plugins::backend::graphics::Orthographic{.zoom = 3.0F}});
    app.app().runFrame();
    ASSERT_EQ(app.state().views.size(), 1U);
    const rtype::plugins::backend::graphics::View& view = app.state().views[0];
    EXPECT_FLOAT_EQ(view.position.x, 40.0F);
    EXPECT_FLOAT_EQ(view.position.y, -12.0F);
    EXPECT_FLOAT_EQ(view.rotation, 0.5F);
    EXPECT_FLOAT_EQ(std::get<rtype::plugins::backend::graphics::Orthographic>(view.projection).zoom, 3.0F);
}

TEST(BackendPlugin, ShowsNoCameraWithoutATransform) {
    BackendApp app;
    app.app().getWorld().spawn(rtype::plugins::backend::graphics::Camera{});
    app.app().runFrame();
    EXPECT_TRUE(app.state().views.empty());
}

TEST(BackendPlugin, AppliesTheCameraEveryFrameItIsDrawn) {
    BackendApp app;
    app.app().getWorld().spawn(rtype::plugins::transform::Transform{}, rtype::plugins::backend::graphics::Camera{});
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.state().views.size(), 2U);
    app.state().framebufferSize = {0, 0};
    app.app().runFrame();
    EXPECT_EQ(app.state().views.size(), 2U);
}

TEST(BackendPlugin, RecordsWhatTheCameraShows) {
    BackendApp app;
    app.app().getWorld().spawn(rtype::plugins::transform::Transform{.position = {1000.0F, 500.0F}},
                               rtype::plugins::backend::graphics::Camera{
                                   .projection = rtype::plugins::backend::graphics::Orthographic{.zoom = 2.0F}});
    app.app().runFrame();
    const auto& visible = app.app().getWorld().getResources().require<rtype::plugins::backend::graphics::VisibleArea>();
    constexpr float kTolerance = 0.01F;
    EXPECT_NEAR(visible.min.x, 800.0F, kTolerance);  // The 800x600 window at zoom 2: 400x300 world units.
    EXPECT_NEAR(visible.min.y, 350.0F, kTolerance);
    EXPECT_NEAR(visible.max.x, 1200.0F, kTolerance);
    EXPECT_NEAR(visible.max.y, 650.0F, kTolerance);
}

TEST(BackendPlugin, RecordsTheDefaultViewWithoutACamera) {
    BackendApp app;
    app.app().runFrame();
    const auto& visible = app.app().getWorld().getResources().require<rtype::plugins::backend::graphics::VisibleArea>();
    EXPECT_EQ(visible.min, (glm::vec2{0.0F, 0.0F}));
    EXPECT_EQ(visible.max, (glm::vec2{800.0F, 600.0F}));
    app.state().framebufferSize = {0, 0};
    app.app().runFrame();
    EXPECT_TRUE(visible.isEmpty());  // Minimized: nothing is shown.
}

TEST(BackendPlugin, DrawsTheQueuedSpritesSortedWithTheEntities) {
    BackendApp app;
    const auto texture = loadTexture(app, "ship.png");
    spawnSprite(app, texture, {5.0F, 0.0F}, 5);
    app.app().runFrame();  // Loads the texture.
    const auto loaded =
        app.app().getWorld().getResources().require<rtype::plugins::backend::TextureAssets>().getTexture(texture);
    ASSERT_TRUE(loaded);
    auto& queue = *app.app().getWorld().getResources().getMut<rtype::plugins::backend::graphics::SpriteQueue>();
    queue.push(0, rtype::plugins::backend::graphics::Sprite{.texture = *loaded, .position = {1.0F, 0.0F}});
    queue.push(9, rtype::plugins::backend::graphics::Sprite{.texture = *loaded, .position = {9.0F, 0.0F}});
    app.state().sprites.clear();
    app.app().runFrame();
    ASSERT_EQ(app.state().sprites.size(), 3U);
    EXPECT_FLOAT_EQ(app.state().sprites[0].position.x, 1.0F);
    EXPECT_FLOAT_EQ(app.state().sprites[1].position.x, 5.0F);
    EXPECT_FLOAT_EQ(app.state().sprites[2].position.x, 9.0F);
    EXPECT_TRUE(queue.getItems().empty());  // Emptied once drawn.
}

TEST(BackendPlugin, DrawsTheQueuedTextsInTheirLayer) {
    BackendApp app;
    const auto texture = loadTexture(app, "ship.png");
    spawnSprite(app, texture, {5.0F, 0.0F}, 5);
    app.app().runFrame();  // Loads the texture.
    const auto loaded =
        app.app().getWorld().getResources().require<rtype::plugins::backend::TextureAssets>().getTexture(texture);
    ASSERT_TRUE(loaded);
    auto& queue = *app.app().getWorld().getResources().getMut<rtype::plugins::backend::graphics::SpriteQueue>();
    queue.push(9, rtype::plugins::backend::graphics::Text{.texture = *loaded, .position = {9.0F, 0.0F}});
    queue.push(0, rtype::plugins::backend::graphics::Text{.texture = *loaded, .position = {1.0F, 0.0F}});
    app.state().drawOrder.clear();
    app.app().runFrame();
    EXPECT_EQ(app.state().drawOrder, (std::vector<char>{'t', 's', 't'}));
    ASSERT_EQ(app.state().texts.size(), 2U);
    EXPECT_FLOAT_EQ(app.state().texts.at(0).position.x, 1.0F);
    EXPECT_FLOAT_EQ(app.state().texts.at(1).position.x, 9.0F);
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
