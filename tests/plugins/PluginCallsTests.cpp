/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PluginCallsTests
*/

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "FakeAudioDevice.hpp"
#include "FakeBackend.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/audio/AudioAssets.hpp"
#include "plugins/audio/AudioCalls.hpp"
#include "plugins/audio/AudioEvents.hpp"
#include "plugins/audio/AudioPlugin.hpp"
#include "plugins/audio/exceptions/AudioCallException.hpp"
#include "plugins/graphics/GraphicsPlugin.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/graphics/TextureCalls.hpp"
#include "plugins/input/InputCalls.hpp"
#include "plugins/input/InputPlugin.hpp"
#include "plugins/input/PlatformEvent.hpp"
#include "plugins/input/exceptions/InputExceptions.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsCalls.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/PhysicsWorld.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/exceptions/PhysicsExceptions.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/SpriteCalls.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"
#include "plugins/text/FontAssets.hpp"
#include "plugins/text/TextCalls.hpp"
#include "plugins/text/TextPlugin.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapCalls.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/contract/audiodevice/AudioTypes.hpp"
#include "rtype/contract/platform/event/Event.hpp"
#include "rtype/contract/platform/input/Key.hpp"
#include "rtype/contract/renderer/drawing/Texture.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallInfo.hpp"
#include "rtype/ecs/call/Calls.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"

using rtype::core::asset::AssetId;

namespace {
/// @brief Makes the engine call @p name of @p app, as a script does while it loads, and returns its result.
/// @throws std::invalid_argument If no call has this name; whatever the call throws.
template <typename Result, typename Arguments>
Result call(rtype::ecs::App& app, const std::string_view name, const Arguments& arguments) {
    rtype::ecs::Calls& calls = app.getWorld().getCalls();
    const rtype::ecs::CallInfo* info = calls.findByName(name);
    if (info == nullptr) {
        throw std::invalid_argument{std::format("No engine call '{}'", name)};
    }
    std::array<std::byte, sizeof(Arguments)> bytes{};
    std::memcpy(bytes.data(), &arguments, sizeof(Arguments));
    if constexpr (std::is_void_v<Result>) {
        calls.invoke(app.getWorld(), info->id, bytes, std::span<std::byte>{});
    } else {
        std::array<std::byte, sizeof(Result)> result{};
        calls.invoke(app.getWorld(), info->id, bytes, result);
        Result value{};
        std::memcpy(&value, result.data(), sizeof(Result));
        return value;
    }
}

/// @brief Sends a key press to the InputPlugin, as the backend plugin does after polling the window.
void press(rtype::ecs::App& app, const rtype::contract::platform::input::Key key) {
    app.getWorld().getEvents().send(
        rtype::plugins::input::PlatformEvent{.event = rtype::contract::platform::event::KeyPressed{.key = key}});
}

/// @return The atlas @p id names in @p app.
const rtype::plugins::sprite::SpriteAtlas& atlasOf(rtype::ecs::App& app, const AssetId id) {
    const auto* atlas = app.getWorld().getResources().require<rtype::plugins::sprite::AtlasAssets>().get(id);
    if (atlas == nullptr) {
        throw std::invalid_argument{"No such atlas"};
    }
    return *atlas;
}
}  // namespace

// ---- input ----

TEST(InputCalls, AddActionAndBindDriveAnAction) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    call<void>(app, rtype::plugins::input::kAddActionCall, rtype::plugins::input::AddActionArguments{.name = "shoot"});
    call<void>(app, rtype::plugins::input::kBindCall,
               rtype::plugins::input::BindArguments{.action = "shoot", .control = "Space"});

    press(app, rtype::contract::platform::input::Key::kSpace);
    app.runFrame();

    const auto state = call<rtype::plugins::input::ActionState>(app, rtype::plugins::input::kReadActionCall,
                                                                rtype::plugins::input::ReadActionArguments{"shoot"});
    EXPECT_TRUE(state.held);
    EXPECT_TRUE(state.pressed);
    EXPECT_FALSE(state.released);
    EXPECT_FLOAT_EQ(state.value.x, 1.0F);
}

TEST(InputCalls, BindVectorTakesFourControlsOrTwo) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    call<void>(app, rtype::plugins::input::kAddActionCall,
               rtype::plugins::input::AddActionArguments{.name = "move", .type = "vector2"});
    call<void>(app, rtype::plugins::input::kBindVectorCall,
               rtype::plugins::input::BindVectorArguments{
                   .action = "move", .up = "W", .down = "S", .left = "A", .right = "D"});
    call<void>(app, rtype::plugins::input::kBindVectorCall,
               rtype::plugins::input::BindVectorArguments{.action = "move", .x = "GamepadLeftX", .y = "GamepadLeftY"});
    EXPECT_THROW(call<void>(app, rtype::plugins::input::kBindVectorCall,
                            rtype::plugins::input::BindVectorArguments{.action = "move"}),
                 rtype::plugins::input::exceptions::InputException);
    EXPECT_THROW(
        call<void>(app, rtype::plugins::input::kBindVectorCall,
                   rtype::plugins::input::BindVectorArguments{.action = "move", .up = "W", .x = "MouseDeltaX"}),
        rtype::plugins::input::exceptions::InputException);

    press(app, rtype::contract::platform::input::Key::kD);
    app.runFrame();

    const auto state = call<rtype::plugins::input::ActionState>(app, rtype::plugins::input::kReadActionCall,
                                                                rtype::plugins::input::ReadActionArguments{"move"});
    EXPECT_FLOAT_EQ(state.value.x, 1.0F);
    EXPECT_FLOAT_EQ(state.value.y, 0.0F);
}

TEST(InputCalls, BindAxisAndScaleBindAControl) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    call<void>(app, rtype::plugins::input::kAddActionCall,
               rtype::plugins::input::AddActionArguments{.name = "steer", .type = "axis"});
    call<void>(app, rtype::plugins::input::kBindAxisCall,
               rtype::plugins::input::BindAxisArguments{.action = "steer", .negative = "Left", .positive = "Right"});
    call<void>(app, rtype::plugins::input::kAddActionCall,
               rtype::plugins::input::AddActionArguments{.name = "boost", .type = "axis"});
    call<void>(app, rtype::plugins::input::kBindCall,
               rtype::plugins::input::BindArguments{.action = "boost", .control = "B", .scale = -2.0F});

    press(app, rtype::contract::platform::input::Key::kLeft);
    press(app, rtype::contract::platform::input::Key::kB);
    app.runFrame();

    const auto steer = call<rtype::plugins::input::ActionState>(app, rtype::plugins::input::kReadActionCall,
                                                                rtype::plugins::input::ReadActionArguments{"steer"});
    EXPECT_FLOAT_EQ(steer.value.x, -1.0F);
    const auto boost = call<rtype::plugins::input::ActionState>(app, rtype::plugins::input::kReadActionCall,
                                                                rtype::plugins::input::ReadActionArguments{"boost"});
    EXPECT_FLOAT_EQ(boost.value.x, -2.0F);
}

TEST(InputCalls, UnknownNamesThrow) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    EXPECT_THROW(call<void>(app, rtype::plugins::input::kAddActionCall,
                            rtype::plugins::input::AddActionArguments{.name = "shoot", .type = "trigger"}),
                 rtype::plugins::input::exceptions::InputException);
    EXPECT_THROW(
        call<void>(app, rtype::plugins::input::kAddActionCall, rtype::plugins::input::AddActionArguments{.name = ""}),
        rtype::plugins::input::exceptions::InputException);
    call<void>(app, rtype::plugins::input::kAddActionCall, rtype::plugins::input::AddActionArguments{.name = "shoot"});
    try {
        call<void>(app, rtype::plugins::input::kBindCall,
                   rtype::plugins::input::BindArguments{.action = "shoot", .control = "Jump"});
        ADD_FAILURE() << "an unknown control should throw";
    } catch (const rtype::plugins::input::exceptions::InputException& error) {
        EXPECT_NE(std::string{error.what()}.find("'Jump'"), std::string::npos) << error.what();
    }
    EXPECT_THROW(static_cast<void>(call<rtype::plugins::input::ActionState>(
                     app, rtype::plugins::input::kReadActionCall, rtype::plugins::input::ReadActionArguments{"fly"})),
                 rtype::plugins::input::exceptions::InputException);
}

// ---- sprite ----

TEST(SpriteCalls, CreateAtlasCutsAGridNamedAfterItsTexture) {
    rtype::ecs::App app;
    app.addPlugin(rtype::plugins::sprite::SpritePlugin{});
    const auto created = call<rtype::plugins::sprite::AtlasResult>(
        app, rtype::plugins::sprite::kCreateAtlasCall,
        rtype::plugins::sprite::CreateAtlasArguments{
            .texture = "ships.png", .frameSize = {10.0F, 10.0F}, .count = 4, .spacing = {2.0F, 0.0F}, .linear = true});

    EXPECT_EQ(created.atlas, AssetId::fromPath("ships.png"));
    const auto& atlas = atlasOf(app, created.atlas);
    ASSERT_EQ(atlas.getFrames().size(), 4U);
    EXPECT_FLOAT_EQ(atlas.getFrames().at(1).source.getX(), 12.0F);
    EXPECT_EQ(atlas.getTextureDesc().filter, rtype::contract::renderer::TextureFilter::kLinear);
}

TEST(SpriteCalls, AddFrameAndAddAnimationFillAnAtlas) {
    rtype::ecs::App app;
    app.addPlugin(rtype::plugins::sprite::SpritePlugin{});
    const AssetId laser = call<rtype::plugins::sprite::AtlasResult>(
                              app, rtype::plugins::sprite::kCreateAtlasCall,
                              rtype::plugins::sprite::CreateAtlasArguments{.name = "laser", .texture = "laser.png"})
                              .atlas;
    EXPECT_EQ(laser, AssetId::fromPath("laser"));
    EXPECT_TRUE(atlasOf(app, laser).getFrames().empty());

    const auto first = call<rtype::plugins::sprite::FrameResult>(
        app, rtype::plugins::sprite::kAddFrameCall,
        rtype::plugins::sprite::AddFrameArguments{
            .atlas = laser, .position = {108.0F, 151.0F}, .size = {80.0F, 16.0F}});
    const auto second = call<rtype::plugins::sprite::FrameResult>(
        app, rtype::plugins::sprite::kAddFrameCall,
        rtype::plugins::sprite::AddFrameArguments{
            .atlas = laser, .name = "tip", .size = {8.0F, 8.0F}, .origin = {0.0F, 0.0F}});
    EXPECT_EQ(first.frame, 0U);
    EXPECT_EQ(second.frame, 1U);
    const auto& frames = atlasOf(app, laser).getFrames();
    EXPECT_EQ(frames.at(0).name, "0");
    EXPECT_FLOAT_EQ(frames.at(0).origin.x, 40.0F);  // not given: the centre
    EXPECT_EQ(frames.at(1).name, "tip");
    EXPECT_FLOAT_EQ(frames.at(1).origin.x, 0.0F);

    call<void>(app, rtype::plugins::sprite::kAddAnimationCall,
               rtype::plugins::sprite::AddAnimationArguments{
                   .atlas = laser, .name = "both", .count = 2, .frameDuration = 0.08F, .direction = "pingPong"});
    call<void>(app, rtype::plugins::sprite::kAddAnimationCall,
               rtype::plugins::sprite::AddAnimationArguments{.atlas = laser, .name = "rest", .first = 1, .plays = 1});
    const auto* both = atlasOf(app, laser).findAnimation(rtype::plugins::sprite::animationId("both"));
    ASSERT_NE(both, nullptr);
    EXPECT_EQ(both->frames, (std::vector<std::uint32_t>{0, 1}));
    EXPECT_EQ(both->direction, rtype::plugins::sprite::AnimationDirection::kPingPong);
    const auto* rest = atlasOf(app, laser).findAnimation(rtype::plugins::sprite::animationId("rest"));
    ASSERT_NE(rest, nullptr);
    EXPECT_EQ(rest->frames, (std::vector<std::uint32_t>{1}));  // count 0: up to the last frame
}

TEST(SpriteCalls, MistakesThrowAnAtlasException) {
    rtype::ecs::App app;
    app.addPlugin(rtype::plugins::sprite::SpritePlugin{});
    EXPECT_THROW(static_cast<void>(call<rtype::plugins::sprite::AtlasResult>(
                     app, rtype::plugins::sprite::kCreateAtlasCall, rtype::plugins::sprite::CreateAtlasArguments{})),
                 rtype::plugins::sprite::exceptions::AtlasException);
    EXPECT_THROW(static_cast<void>(call<rtype::plugins::sprite::FrameResult>(
                     app, rtype::plugins::sprite::kAddFrameCall,
                     rtype::plugins::sprite::AddFrameArguments{.atlas = AssetId::fromPath("nowhere")})),
                 rtype::plugins::sprite::exceptions::AtlasException);
    const AssetId sheet =
        call<rtype::plugins::sprite::AtlasResult>(
            app, rtype::plugins::sprite::kCreateAtlasCall,
            rtype::plugins::sprite::CreateAtlasArguments{.texture = "sheet.png", .frameSize = {4.0F, 4.0F}, .count = 2})
            .atlas;
    EXPECT_THROW(call<void>(app, rtype::plugins::sprite::kAddAnimationCall,
                            rtype::plugins::sprite::AddAnimationArguments{
                                .atlas = sheet, .name = "spin", .direction = "sideways"}),
                 rtype::plugins::sprite::exceptions::AtlasException);
    EXPECT_THROW(call<void>(app, rtype::plugins::sprite::kAddAnimationCall,
                            rtype::plugins::sprite::AddAnimationArguments{.atlas = sheet, .name = "far", .first = 5}),
                 rtype::plugins::sprite::exceptions::AtlasException);
}

// ---- texture, font, tilemap ----

TEST(TextureCalls, LoadTextureAsksForTheImageWithItsSampling) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    const auto state = std::make_shared<rtype::test::plugins::FakeBackendState>();
    app.addPlugin(rtype::plugins::transform::TransformPlugin{});
    app.addPlugin(rtype::plugins::input::InputPlugin{});
    app.addPlugin(rtype::plugins::graphics::GraphicsPlugin{rtype::test::plugins::makeFakeRegistry(state),
                                                           rtype::test::plugins::makeFakeSettings()});
    const auto loaded = call<rtype::plugins::graphics::TextureResult>(
        app, rtype::plugins::graphics::kLoadTextureCall,
        rtype::plugins::graphics::LoadTextureArguments{.path = "planet.png", .linear = true, .wrap = true});

    EXPECT_EQ(loaded.texture, AssetId::fromPath("planet.png"));
    const auto* entry =
        app.getWorld().getResources().require<rtype::plugins::graphics::TextureAssets>().get(loaded.texture);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->desc.filter, rtype::contract::renderer::TextureFilter::kLinear);
    EXPECT_TRUE(entry->desc.repeat);
}

TEST(TextCalls, LoadFontAsksForTheFont) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    const auto state = std::make_shared<rtype::test::plugins::FakeBackendState>();
    rtype::test::plugins::addDrawingPlugins(app, rtype::test::plugins::makeFakeRegistry(state),
                                            rtype::test::plugins::makeFakeSettings());
    const auto loaded = call<rtype::plugins::text::FontResult>(
        app, rtype::plugins::text::kLoadFontCall, rtype::plugins::text::LoadFontArguments{"missing-font.json"});

    EXPECT_EQ(loaded.font, AssetId::fromPath("missing-font.json"));
    EXPECT_NE(app.getWorld().getResources().require<rtype::plugins::text::FontAssets>().getEntry(loaded.font), nullptr);
}

TEST(TilemapCalls, SpawningAMapThatFailedToLoadSpawnsNothing) {
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};
    const auto state = std::make_shared<rtype::test::plugins::FakeBackendState>();
    rtype::test::plugins::addDrawingPlugins(app, rtype::test::plugins::makeFakeRegistry(state),
                                            rtype::test::plugins::makeFakeSettings());
    const auto loaded = call<rtype::plugins::tilemap::TilemapResult>(
        app, rtype::plugins::tilemap::kLoadTilemapCall, rtype::plugins::tilemap::LoadTilemapArguments{"missing.tmj"});
    EXPECT_EQ(loaded.map, AssetId::fromPath("missing.tmj"));
    EXPECT_NE(app.getWorld().getResources().require<rtype::plugins::tilemap::TilemapAssets>().getEntry(loaded.map),
              nullptr);

    const auto spawned = call<rtype::plugins::tilemap::SpawnTilemapResult>(
        app, rtype::plugins::tilemap::kSpawnTilemapCall, rtype::plugins::tilemap::SpawnTilemapArguments{loaded.map});
    EXPECT_EQ(spawned.layers, 0U);
}

// ---- audio ----

namespace {
/// @brief An App with the AudioPlugin over the fake device.
struct AudioCallsApp {
    std::shared_ptr<rtype::test::plugins::FakeAudioState> state{
        std::make_shared<rtype::test::plugins::FakeAudioState>()};
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};

    AudioCallsApp() {
        app.addPlugin(rtype::plugins::transform::TransformPlugin{});
        app.addPlugin(rtype::plugins::audio::AudioPlugin{rtype::test::plugins::makeFakeAudioRegistry(state),
                                                         rtype::test::plugins::makeFakeAudioSettings()});
    }
};
}  // namespace

TEST(AudioCalls, LoadSoundAsksForTheSound) {
    AudioCallsApp audio;
    const auto loaded = call<rtype::plugins::audio::SoundResult>(
        audio.app, rtype::plugins::audio::kLoadSoundCall,
        rtype::plugins::audio::LoadSoundArguments{.path = "music.ogg", .stream = true});

    EXPECT_EQ(loaded.sound, AssetId::fromPath("music.ogg"));
    const auto* entry =
        audio.app.getWorld().getResources().require<rtype::plugins::audio::AudioAssets>().get(loaded.sound);
    ASSERT_NE(entry, nullptr);
    EXPECT_TRUE(entry->desc.stream);
}

TEST(AudioCalls, PlaySoundSendsAPlaySound) {
    AudioCallsApp audio;
    const AssetId shot = AssetId::fromPath("shot.wav");
    call<void>(audio.app, rtype::plugins::audio::kPlaySoundCall,
               rtype::plugins::audio::PlaySoundArguments{.sound = shot, .bus = "music", .volume = 0.5F});
    call<void>(audio.app, rtype::plugins::audio::kPlaySoundCall,
               rtype::plugins::audio::PlaySoundArguments{.sound = shot, .position = {3.0F, 4.0F}});
    EXPECT_THROW(call<void>(audio.app, rtype::plugins::audio::kPlaySoundCall,
                            rtype::plugins::audio::PlaySoundArguments{.sound = shot, .bus = "voice"}),
                 rtype::plugins::audio::exceptions::AudioCallException);

    rtype::ecs::EventCursor cursor;
    std::vector<rtype::plugins::audio::PlaySound> sent;
    for (const rtype::plugins::audio::PlaySound& sound :
         audio.app.getWorld().getEvents().read<rtype::plugins::audio::PlaySound>(cursor)) {
        sent.push_back(sound);
    }
    ASSERT_EQ(sent.size(), 2U);
    EXPECT_EQ(sent[0].sound, shot);
    EXPECT_EQ(sent[0].bus, rtype::contract::audiodevice::Bus::kMusic);
    EXPECT_FLOAT_EQ(sent[0].volume, 0.5F);
    EXPECT_FALSE(sent[0].position);  // not given: centred
    EXPECT_EQ(sent[1].bus, rtype::contract::audiodevice::Bus::kEffects);
    ASSERT_TRUE(sent[1].position);
    EXPECT_FLOAT_EQ(sent[1].position->y, 4.0F);
}

// ---- physics ----

namespace {
namespace physics = rtype::plugins::physics;

constexpr float kTolerance = 0.05F;
constexpr std::uint32_t kWallLayer = 1U << 1;

/// @brief An App with the physics plugin and a static wall across the +X axis, its left face at x = 90.
struct PhysicsCallsApp {
    rtype::ecs::App app{rtype::ecs::RunMode::kServer};
    rtype::ecs::Entity wall;

    PhysicsCallsApp() {
        app.addPlugin(rtype::plugins::transform::TransformPlugin{});
        app.addPlugin(physics::PhysicsPlugin{physics::PhysicsSettings{
            .maxBodies = 16, .maxBodyPairs = 16, .maxContactConstraints = 16, .tempAllocatorBytes = 1024 * 1024}});
        const physics::Collider collider{.size = {20.0F, 200.0F}, .layer = kWallLayer};
        const rtype::plugins::transform::Transform at{.position = {100.0F, 0.0F}};
        wall = app.getWorld().spawn(collider, at);
        static_cast<void>(world()->createBody(wall, collider, nullptr, at));
    }

    /// @return The physics world, to give bodies without stepping the App.
    [[nodiscard]] rtype::ecs::Mut<physics::PhysicsWorld> world() {
        return app.getWorld().getResources().requireMut<physics::PhysicsWorld>();
    }
};
}  // namespace

TEST(PhysicsCalls, RaycastHitsTheNearestBodyItsFilterSees) {
    PhysicsCallsApp physics;
    const auto hit =
        call<physics::HitResult>(physics.app, physics::kRaycastCall, physics::RaycastArguments{.maxDistance = 1000.0F});
    ASSERT_TRUE(hit.hit);
    EXPECT_EQ(hit.entity, physics.wall);
    EXPECT_NEAR(hit.point.x, 90.0F, kTolerance);
    EXPECT_NEAR(hit.normal.x, -1.0F, kTolerance);
    EXPECT_NEAR(hit.distance, 90.0F, kTolerance);

    const auto ignored = call<physics::HitResult>(
        physics.app, physics::kRaycastCall, physics::RaycastArguments{.maxDistance = 1000.0F, .ignore = physics.wall});
    EXPECT_FALSE(ignored.hit);
    EXPECT_TRUE(ignored.entity.isNull());
    const auto masked = call<physics::HitResult>(physics.app, physics::kRaycastCall,
                                                 physics::RaycastArguments{.maxDistance = 1000.0F, .mask = 1U << 3});
    EXPECT_FALSE(masked.hit);
}

TEST(PhysicsCalls, ShapeCastAndOverlapUseTheShapeFields) {
    PhysicsCallsApp physics;
    const auto cast = call<physics::HitResult>(
        physics.app, physics::kShapeCastCall,
        physics::ShapeCastArguments{.shape = physics::ShapeType::kCircle, .radius = 10.0F, .maxDistance = 1000.0F});
    ASSERT_TRUE(cast.hit);
    EXPECT_EQ(cast.entity, physics.wall);
    EXPECT_NEAR(cast.distance, 80.0F, kTolerance);  // the circle's edge touches the face at x = 90

    const auto inside = call<physics::OverlapResult>(physics.app, physics::kOverlapCall,
                                                     physics::OverlapArguments{.at = {100.0F, 0.0F}});
    EXPECT_EQ(inside.count, 1U);
    EXPECT_EQ(inside.first, physics.wall);
    const auto outside = call<physics::OverlapResult>(physics.app, physics::kOverlapCall,
                                                      physics::OverlapArguments{.at = {500.0F, 0.0F}});
    EXPECT_EQ(outside.count, 0U);
    EXPECT_TRUE(outside.first.isNull());
    EXPECT_THROW(
        call<physics::OverlapResult>(physics.app, physics::kOverlapCall,
                                     physics::OverlapArguments{.shape = physics::ShapeType::kCircle, .radius = -1.0F}),
        physics::exceptions::PhysicsException);
}

TEST(PhysicsCalls, MoveAndSlideStopsAnEntityAgainstAWall) {
    PhysicsCallsApp physics;
    const rtype::ecs::Entity ship =
        physics.app.getWorld().spawn(physics::Collider{.shape = physics::ShapeType::kCircle, .radius = 10.0F});

    const auto moved = call<physics::MoveAndSlideResult>(
        physics.app, physics::kMoveAndSlideCall,
        physics::MoveAndSlideArguments{.entity = ship, .delta = {200.0F, 0.0F}, .skin = 0.5F});
    EXPECT_NEAR(moved.position.x, 79.5F, kTolerance);  // a skin away from the wall
    EXPECT_NEAR(moved.position.y, 0.0F, kTolerance);

    const rtype::ecs::Entity ghost = physics.app.getWorld().spawn();
    EXPECT_THROW(call<physics::MoveAndSlideResult>(physics.app, physics::kMoveAndSlideCall,
                                                   physics::MoveAndSlideArguments{.entity = ghost}),
                 physics::exceptions::PhysicsException);
}

TEST(PhysicsCalls, PushesAndGravityReachTheWorld) {
    PhysicsCallsApp physics;
    call<void>(physics.app, physics::kSetGravityCall, physics::GravityArguments{.gravity = {0.0F, 0.0F}});
    EXPECT_FLOAT_EQ(physics.world()->getGravity().y, 0.0F);

    const physics::Collider collider{.shape = physics::ShapeType::kCircle, .radius = 5.0F};
    const rtype::plugins::transform::Transform at{.position = {0.0F, 300.0F}};
    const rtype::ecs::Entity ball = physics.app.getWorld().spawn(collider, at, physics::RigidBody{});
    const physics::RigidBody body{.linearDamping = 0.0F};
    static_cast<void>(physics.world()->createBody(ball, collider, &body, at));

    call<void>(physics.app, physics::kApplyImpulseCall, physics::PushArguments{.entity = ball, .value = {10.0F, 0.0F}});
    physics.world()->step(1.0F / 60.0F);
    const auto velocity = physics.world()->getBodyVelocity(ball);
    ASSERT_TRUE(velocity);
    EXPECT_NEAR(velocity->linear.x, 10.0F, kTolerance);  // one kilogram: the impulse is the change of velocity

    call<void>(physics.app, physics::kApplyForceCall, physics::PushArguments{.entity = ball, .value = {0.0F, 60.0F}});
    call<void>(physics.app, physics::kApplyTorqueCall, physics::TorqueArguments{.entity = ball, .torque = 1.0F});
    physics.world()->step(1.0F / 60.0F);
    const auto pushed = physics.world()->getBodyVelocity(ball);
    ASSERT_TRUE(pushed);
    EXPECT_NEAR(pushed->linear.y, 1.0F, kTolerance);  // 60 N for a sixtieth of a second
    EXPECT_GT(pushed->angular, 0.0F);
}
