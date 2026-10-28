/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TilemapPluginTests
*/

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
#include <memory>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/graphics/Camera.hpp"
#include "plugins/backend/graphics/SpriteQueue.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/backend/graphics/VisibleArea.hpp"
#include "plugins/tilemap/Parallax.hpp"
#include "plugins/tilemap/SpawnTilemap.hpp"
#include "plugins/tilemap/Tilemap.hpp"
#include "plugins/tilemap/TilemapAssets.hpp"
#include "plugins/tilemap/TilemapLayer.hpp"
#include "plugins/tilemap/TilemapPlugin.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/command/Commands.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/query/Query.hpp"
#include "rtype/ecs/query/QueryState.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/system/ResourceParam.hpp"
#include "rtype/ecs/system/Side.hpp"
#include "rtype/ecs/time/TimePlugin.hpp"
#include "rtype/ecs/world/World.hpp"

using rtype::plugins::asset::AssetId;
using rtype::plugins::tilemap::LayerKind;
using rtype::plugins::tilemap::MapLayer;
using rtype::plugins::tilemap::Parallax;
using rtype::plugins::tilemap::Tilemap;
using rtype::plugins::tilemap::TilemapAssets;
using rtype::plugins::tilemap::TilemapLayer;
using rtype::plugins::tilemap::Tileset;
using rtype::plugins::transform::GlobalTransform;
using rtype::plugins::transform::Transform;

namespace {
/// @brief The name the test map is added under.
constexpr const char* kLevel = "level";

/// @brief Startup: spawns the layers of the map "level", their draw layers from 5.
void spawnLevel(rtype::ecs::ResMut<TilemapAssets> maps, rtype::ecs::Commands& commands) {
    static_cast<void>(rtype::plugins::tilemap::spawnTilemap(
        commands, *maps, AssetId::fromPath(kLevel), rtype::plugins::tilemap::TilemapPlacement{.firstDrawLayer = 5}));
}

/// @brief An App with the BackendPlugin over the fake backend (an 800x600 window), frames of one second, and the
/// image tiles.png (2x1 pixels: a tileset of two 1x1 tiles) loaded.
class TilemapApp {
  public:
    TilemapApp()
        : _state{std::make_shared<rtype::test::plugins::FakeBackendState>()}, _app{rtype::ecs::RunMode::kClient} {
        _app.addPlugin(rtype::plugins::backend::BackendPlugin{rtype::test::plugins::makeFakeRegistry(_state),
                                                              rtype::test::plugins::makeFakeSettings()});
        _app.addPlugin(rtype::ecs::TimePlugin::manual(std::chrono::seconds{1}));
        static_cast<void>(world().getResources().getMut<rtype::plugins::backend::TextureAssets>()->load(
            "tiles.png", rtype::test::plugins::twoPixelPng()));
    }
    ~TilemapApp() = default;
    TilemapApp(const TilemapApp& other) = delete;
    TilemapApp& operator=(const TilemapApp& other) = delete;
    TilemapApp(TilemapApp&& other) = delete;
    TilemapApp& operator=(TilemapApp&& other) = delete;

    [[nodiscard]] rtype::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] rtype::ecs::World& world() noexcept { return _app.getWorld(); }
    [[nodiscard]] rtype::test::plugins::FakeBackendState& state() noexcept { return *_state; }

    /// @brief Adds @p layers as the map "level", over a 16x16 grid with the tileset tiles.png, and spawns it.
    void spawnMap(std::vector<MapLayer> layers) {
        const Tileset tiles{.imagePath = "tiles.png",
                            .texture = AssetId::fromPath("tiles.png"),
                            .imageSize = {2, 1},
                            .tileSize = {1, 1},
                            .tileCount = 2,
                            .columns = 2};
        static_cast<void>(
            world().getResources().getMut<TilemapAssets>()->add(kLevel, Tilemap{{16, 16}, {tiles}, std::move(layers)}));
        _app.addSystem(rtype::ecs::Phase::kStartup, "test.spawnLevel", &spawnLevel).setSide(rtype::ecs::Side::kBoth);
    }

    /// @return The sprites drawn by one more frame.
    std::vector<rtype::plugins::backend::graphics::Sprite> drawFrame() {
        _state->sprites.clear();
        _app.runFrame();
        return _state->sprites;
    }

  private:
    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    rtype::ecs::App _app;                                            ///< The App under test.
};

/// @return A tile layer with the tile id 1 in each of @p cells.
MapLayer tilesAt(const std::vector<glm::ivec2>& cells) {
    MapLayer layer{.name = "ground"};
    const std::array<std::uint32_t, 1> tile{1};
    for (const glm::ivec2 cell : cells) {
        rtype::plugins::tilemap::setTiles(layer, cell, 1, tile);
    }
    return layer;
}

/// @return The entities that have a TilemapLayer, with it.
std::vector<std::pair<rtype::ecs::Entity, TilemapLayer>> layersOf(rtype::ecs::World& world) {
    std::vector<std::pair<rtype::ecs::Entity, TilemapLayer>> layers;
    const rtype::ecs::QueryState<const TilemapLayer> state{world.getComponents()};
    rtype::ecs::Query<const TilemapLayer> query{state, 0};
    for (auto [entity, layer] : query) {
        layers.emplace_back(entity, layer);
    }
    return layers;
}
}  // namespace

TEST(TilemapPlugin, ComesWithTheBackendPlugin) {
    TilemapApp app;
    EXPECT_TRUE(app.app().hasPlugin(rtype::plugins::tilemap::TilemapPlugin::kName));
    EXPECT_TRUE(app.world().getResources().has<TilemapAssets>());
    EXPECT_TRUE(app.world().getResources().has<rtype::plugins::backend::graphics::SpriteQueue>());
    EXPECT_TRUE(app.world().getResources().has<rtype::plugins::backend::graphics::VisibleArea>());
    EXPECT_TRUE(app.world().getComponents().find("rtype.TilemapLayer"));
    EXPECT_TRUE(app.world().getComponents().find("rtype.Parallax"));
}

TEST(TilemapPlugin, SpawnsOneEntityPerLayer) {
    TilemapApp app;
    MapLayer sky{.name = "sky", .kind = LayerKind::kImage, .offset = {4.0F, 0.0F}, .parallax = {0.5F, 1.0F}};
    app.spawnMap({sky, tilesAt({{0, 0}})});
    app.app().runFrame();
    const auto layers = layersOf(app.world());
    ASSERT_EQ(layers.size(), 2U);
    for (const auto& [entity, layer] : layers) {
        EXPECT_EQ(layer.map, AssetId::fromPath(kLevel));
        EXPECT_EQ(layer.drawLayer, 5 + static_cast<std::int32_t>(layer.layer));
        if (layer.layer == 0) {
            EXPECT_EQ(app.world().getComponents().get<Transform>(entity)->position, (glm::vec2{4.0F, 0.0F}));
            EXPECT_EQ(app.world().getComponents().get<Parallax>(entity)->factor, (glm::vec2{0.5F, 1.0F}));
        }
    }
}

TEST(TilemapPlugin, DrawsOnlyTheChunksTheCameraShows) {
    TilemapApp app;
    app.spawnMap({tilesAt({{0, 0}, {1, 0}, {100, 0}})});  // The last one at x 1600, past the 800-pixel view.
    const auto sprites = app.drawFrame();
    ASSERT_EQ(sprites.size(), 2U);
    EXPECT_EQ(sprites.at(0).position, (glm::vec2{0.5F, 15.5F}));  // A 1x1 tile at the bottom-left of its cell.
    EXPECT_EQ(sprites.at(1).position, (glm::vec2{16.5F, 15.5F}));
}

TEST(TilemapPlugin, SkipsAHiddenLayer) {
    TilemapApp app;
    MapLayer hidden = tilesAt({{0, 0}});
    hidden.visible = false;
    app.spawnMap({hidden});
    EXPECT_TRUE(app.drawFrame().empty());
}

TEST(TilemapPlugin, RepeatsAnImageLayerAcrossTheView) {
    TilemapApp app;
    app.spawnMap({MapLayer{.kind = LayerKind::kImage,
                           .imagePath = "tiles.png",
                           .image = AssetId::fromPath("tiles.png"),
                           .repeatX = true}});
    const auto sprites = app.drawFrame();
    ASSERT_EQ(sprites.size(), 1U);
    EXPECT_FLOAT_EQ(sprites.at(0).source.getWidth(), 800.0F);  // 400 copies of the 2-pixel image.
    EXPECT_FLOAT_EQ(sprites.at(0).source.getHeight(), 1.0F);
}

TEST(TilemapPlugin, SortsTheTilesWithTheSpritesByLayer) {
    TilemapApp app;
    app.spawnMap({tilesAt({{0, 0}})});  // Draw layer 5.
    const rtype::ecs::Entity ship = app.world().spawn();
    app.world().getComponents().add(ship, Transform{.position = {300.0F, 300.0F}});
    app.world().getComponents().add(
        ship, rtype::plugins::backend::SpriteRenderer{.texture = AssetId::fromPath("tiles.png"), .layer = 10});
    const auto sprites = app.drawFrame();
    ASSERT_EQ(sprites.size(), 2U);
    EXPECT_EQ(sprites.at(1).position, (glm::vec2{300.0F, 300.0F}));  // Layer 10, on top of the tiles.
}

TEST(TilemapPlugin, MovesAParallaxEntityByTheCamera) {
    TilemapApp app;
    const rtype::ecs::Entity camera = app.world().spawn();
    app.world().getComponents().add(camera, Transform{.position = {400.0F, 300.0F}});
    app.world().getComponents().add(camera, rtype::plugins::backend::graphics::Camera{});
    const rtype::ecs::Entity far = app.world().spawn();
    app.world().getComponents().add(far, Transform{.position = {10.0F, 0.0F}});
    app.world().getComponents().add(far, Parallax{.factor = {0.5F, 0.0F}, .origin = {0.0F, 100.0F}});
    app.app().runFrame();
    // (camera - origin) * (1 - factor): (400, 200) * (0.5, 1).
    EXPECT_EQ(app.world().getComponents().get<GlobalTransform>(far)->position, (glm::vec2{210.0F, 200.0F}));
}

TEST(TilemapPlugin, ScrollsAParallaxEntityOnItsOwn) {
    TilemapApp app;
    const rtype::ecs::Entity cloud = app.world().spawn();
    app.world().getComponents().add(cloud, Transform{});
    app.world().getComponents().add(cloud, Parallax{.scroll = {-3.0F, 0.0F}});
    app.app().runFrame();
    app.app().runFrame();
    EXPECT_EQ(app.world().getComponents().get<Parallax>(cloud)->scrolled, (glm::vec2{-6.0F, 0.0F}));
    EXPECT_EQ(app.world().getComponents().get<GlobalTransform>(cloud)->position, (glm::vec2{-6.0F, 0.0F}));
}
