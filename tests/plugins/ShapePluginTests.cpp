/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** ShapePluginTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <utility>
#include <vector>

#include "FakeBackend.hpp"
#include "TestImages.hpp"
#include "core/asset/AssetId.hpp"
#include "plugins/graphics/TextureAssets.hpp"
#include "plugins/shape/ShapeRenderer.hpp"
#include "plugins/sprite/SpriteRenderer.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/contract/renderer/drawing/Color.hpp"
#include "rtype/contract/renderer/drawing/Shape.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/world/World.hpp"

namespace {
/// @brief A client App with every drawing plugin, the ShapePlugin included, over the fake platform and renderer.
class ShapeApp {
  public:
    ShapeApp()
        : _state{std::make_shared<rtype::test::plugins::FakeBackendState>()}, _app{rtype::ecs::RunMode::kClient} {
        rtype::test::plugins::addDrawingPlugins(_app, rtype::test::plugins::makeFakeRegistry(_state),
                                                rtype::test::plugins::makeFakeSettings());
    }
    ~ShapeApp() = default;
    ShapeApp(const ShapeApp& other) = delete;
    ShapeApp& operator=(const ShapeApp& other) = delete;
    ShapeApp(ShapeApp&& other) = delete;
    ShapeApp& operator=(ShapeApp&& other) = delete;

    [[nodiscard]] rtype::ecs::App& app() noexcept { return _app; }
    [[nodiscard]] rtype::test::plugins::FakeBackendState& state() noexcept { return *_state; }

  private:
    std::shared_ptr<rtype::test::plugins::FakeBackendState> _state;  ///< Drives the fakes.
    rtype::ecs::App _app;                                            ///< The App under test.
};
}  // namespace

TEST(ShapePlugin, RegistersShapeRenderer) {
    ShapeApp app;
    EXPECT_TRUE(app.app().getWorld().getComponents().find("rtype.ShapeRenderer"));
}

TEST(ShapePlugin, DrawsTheShapesAtTheirGlobalTransformOverTheSprites) {
    ShapeApp app;
    rtype::ecs::World& world = app.app().getWorld();
    const rtype::core::asset::AssetId ship =
        world.getResources().getMut<rtype::plugins::graphics::TextureAssets>()->load(
            "ship.png", rtype::test::plugins::twoPixelPng());
    const rtype::ecs::Entity sprite = world.spawn();
    world.getComponents().add(sprite, rtype::plugins::transform::Transform{});
    world.getComponents().add(sprite, rtype::plugins::sprite::SpriteRenderer{.texture = ship, .layer = 100});
    const rtype::ecs::Entity wall = world.spawn();
    world.getComponents().add(wall, rtype::plugins::transform::Transform{
                                        .position = {10.0F, 20.0F}, .rotation = 0.5F, .scale = {2.0F, 3.0F}});
    world.getComponents().add(wall, rtype::plugins::shape::ShapeRenderer{.size = {4.0F, 5.0F},
                                                                         .cornerRadius = 1.0F,
                                                                         .fillColor = {0.5F, 0.5F, 0.5F, 1.0F},
                                                                         .outlineThickness = 2.0F});
    app.state().drawOrder.clear();
    app.app().runFrame();
    EXPECT_EQ(app.state().drawOrder, (std::vector<char>{'s', 'h'}));  // a shape is an overlay, whatever the layers
    ASSERT_EQ(app.state().shapes.size(), 1U);
    const rtype::contract::renderer::Shape& shape = app.state().shapes.at(0);
    EXPECT_EQ(shape.kind, rtype::contract::renderer::ShapeKind::kRect);
    EXPECT_EQ(shape.position, (glm::vec2{10.0F, 20.0F}));
    EXPECT_FLOAT_EQ(shape.rotation, 0.5F);
    EXPECT_EQ(shape.size, (glm::vec2{8.0F, 15.0F}));  // the size scales on both axes...
    EXPECT_FLOAT_EQ(shape.cornerRadius, 2.0F);        // ... the radii and the outline on X
    EXPECT_FLOAT_EQ(shape.outlineThickness, 4.0F);
    EXPECT_EQ(shape.fillColor, (rtype::contract::renderer::Color{0.5F, 0.5F, 0.5F, 1.0F}));
}

TEST(ShapePlugin, DrawsALineToItsEndInTheEntitysSpace) {
    ShapeApp app;
    rtype::ecs::World& world = app.app().getWorld();
    const rtype::ecs::Entity line = world.spawn();
    world.getComponents().add(line, rtype::plugins::transform::Transform{
                                        .position = {10.0F, 0.0F}, .rotation = 1.5707964F, .scale = {2.0F, 2.0F}});
    world.getComponents().add(
        line, rtype::plugins::shape::ShapeRenderer{
                  .kind = rtype::contract::renderer::ShapeKind::kLine, .end = {5.0F, 0.0F}, .radius = 1.0F});
    app.app().runFrame();
    ASSERT_EQ(app.state().shapes.size(), 1U);
    const rtype::contract::renderer::Shape& shape = app.state().shapes.at(0);
    EXPECT_EQ(shape.position, (glm::vec2{10.0F, 0.0F}));
    EXPECT_NEAR(shape.end.x, 10.0F, 1e-4F);  // turned a quarter clockwise on screen: +X becomes +Y (down)
    EXPECT_NEAR(shape.end.y, 10.0F, 1e-4F);
    EXPECT_FLOAT_EQ(shape.radius, 2.0F);
}

TEST(ShapePlugin, DrawsTheShapesByLayer) {
    ShapeApp app;
    rtype::ecs::World& world = app.app().getWorld();
    for (const auto& [x, layer] : {std::pair{3.0F, 5}, std::pair{1.0F, -2}, std::pair{2.0F, 0}}) {
        const rtype::ecs::Entity entity = world.spawn();
        world.getComponents().add(entity, rtype::plugins::transform::Transform{.position = {x, 0.0F}});
        world.getComponents().add(entity, rtype::plugins::shape::ShapeRenderer{.size = {1.0F, 1.0F}, .layer = layer});
    }
    app.app().runFrame();
    ASSERT_EQ(app.state().shapes.size(), 3U);
    EXPECT_FLOAT_EQ(app.state().shapes[0].position.x, 1.0F);
    EXPECT_FLOAT_EQ(app.state().shapes[1].position.x, 2.0F);
    EXPECT_FLOAT_EQ(app.state().shapes[2].position.x, 3.0F);
}
