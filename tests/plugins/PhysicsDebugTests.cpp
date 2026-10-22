/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** PhysicsDebugTests
*/

#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <memory>
#include <optional>
#include <vector>

#include "FakeBackend.hpp"
#include "plugins/backend/BackendPlugin.hpp"
#include "plugins/backend/event/Event.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/graphics/Shape.hpp"
#include "plugins/backend/input/Key.hpp"
#include "plugins/physics/Collider.hpp"
#include "plugins/physics/PhysicsHandle.hpp"
#include "plugins/physics/PhysicsPlugin.hpp"
#include "plugins/physics/PhysicsSettings.hpp"
#include "plugins/physics/RigidBody.hpp"
#include "plugins/physics/Velocity.hpp"
#include "plugins/physics/debug/PhysicsDebug.hpp"
#include "plugins/physics/debug/PhysicsDebugPlugin.hpp"
#include "plugins/physics/debug/PhysicsDebugSettings.hpp"
#include "plugins/physics/debug/PhysicsDebugShapes.hpp"
#include "plugins/transform/Transform.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/change/Mut.hpp"
#include "rtype/ecs/component/Components.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/time/Time.hpp"

using rtype::ecs::Entity;
using rtype::plugins::backend::graphics::Color;
using rtype::plugins::backend::graphics::Shape;
using rtype::plugins::backend::graphics::ShapeKind;
using rtype::plugins::physics::Collider;
using rtype::plugins::physics::ShapeType;
using rtype::plugins::physics::Velocity;
using rtype::plugins::physics::debug::PhysicsDebug;
using rtype::plugins::physics::debug::PhysicsDebugPlugin;
using rtype::plugins::physics::debug::PhysicsDebugSettings;
using rtype::plugins::transform::GlobalTransform;
using rtype::plugins::transform::Transform;

namespace {
constexpr float kTolerance = 1e-4F;
constexpr Color kRed{1.0F, 0.0F, 0.0F, 1.0F};

/// @brief A client App with the BackendPlugin over the fake renderer and the PhysicsDebugPlugin.
struct DebugApp {
    std::shared_ptr<rtype::test::plugins::FakeBackendState> state =
        std::make_shared<rtype::test::plugins::FakeBackendState>();  ///< Drives the fakes, records the shapes.
    rtype::ecs::App app{rtype::ecs::RunMode::kClient};               ///< The App under test.

    DebugApp() {
        app.addPlugin(rtype::plugins::backend::BackendPlugin{rtype::test::plugins::makeFakeRegistry(state),
                                                             rtype::test::plugins::makeFakeSettings()});
        app.addPlugin(PhysicsDebugPlugin{});
    }

    [[nodiscard]] rtype::ecs::Components& components() { return app.getWorld().getComponents(); }
    [[nodiscard]] rtype::ecs::Mut<PhysicsDebugSettings> settings() {
        return app.getWorld().getResources().requireMut<PhysicsDebugSettings>();
    }

    /// @return A new entity at the position with a box collider, and the debug component when given.
    Entity spawn(glm::vec2 position, std::optional<PhysicsDebug> debug) {
        const Entity entity = app.getWorld().spawn();
        components().add(entity, Transform{.position = position});
        components().add(entity, Collider{.shape = ShapeType::kBox, .size = {40.0F, 20.0F}});
        if (debug) {
            components().add(entity, *debug);
        }
        return entity;
    }

    /// @return The shapes drawn by one more frame.
    std::vector<Shape> frame() {
        state->shapes.clear();
        app.runFrame();
        return state->shapes;
    }
};
}  // namespace

TEST(PhysicsDebugShapes, ABoxOutlineTurnsAndMovesWithItsEntity) {
    const float quarterTurn = std::acos(-1.0F) / 2.0F;
    const Shape shape = rtype::plugins::physics::debug::debugColliderShape(
        Collider{.shape = ShapeType::kBox, .size = {40.0F, 20.0F}, .offset = {10.0F, 0.0F}},
        GlobalTransform{.position = {100.0F, 50.0F}, .rotation = quarterTurn}, kRed, 2.0F);
    EXPECT_EQ(shape.kind, ShapeKind::kRect);
    EXPECT_EQ(shape.size, (glm::vec2{40.0F, 20.0F}));
    EXPECT_FLOAT_EQ(shape.rotation, quarterTurn);
    // The offset turns with the entity: a quarter turn clockwise takes +X to +Y (down).
    EXPECT_NEAR(shape.position.x, 100.0F, kTolerance);
    EXPECT_NEAR(shape.position.y, 60.0F, kTolerance);
    EXPECT_FLOAT_EQ(shape.fillColor.w, 0.0F);  // Outline only.
    EXPECT_EQ(shape.outlineColor, kRed);
    EXPECT_FLOAT_EQ(shape.outlineThickness, 2.0F);
}

TEST(PhysicsDebugShapes, CirclesAndCapsulesKeepTheirSizes) {
    const Shape circle = rtype::plugins::physics::debug::debugColliderShape(
        Collider{.shape = ShapeType::kCircle, .radius = 8.0F}, GlobalTransform{}, kRed, 1.0F);
    EXPECT_EQ(circle.kind, ShapeKind::kCircle);
    EXPECT_FLOAT_EQ(circle.radius, 8.0F);

    const Shape capsule = rtype::plugins::physics::debug::debugColliderShape(
        Collider{.shape = ShapeType::kCapsule, .radius = 6.0F, .halfHeight = 12.0F}, GlobalTransform{}, kRed, 1.0F);
    EXPECT_EQ(capsule.kind, ShapeKind::kCapsule);
    EXPECT_FLOAT_EQ(capsule.radius, 6.0F);
    EXPECT_FLOAT_EQ(capsule.halfHeight, 12.0F);
}

TEST(PhysicsDebugShapes, ASensorIsDrawnFainter) {
    const Shape shape =
        rtype::plugins::physics::debug::debugColliderShape(Collider{.sensor = true}, GlobalTransform{}, kRed, 1.0F);
    EXPECT_FLOAT_EQ(shape.outlineColor.w, 0.5F);
}

TEST(PhysicsDebugShapes, AVelocityArrowIsAShaftAndTwoHeadStrokes) {
    const std::vector<Shape> arrow =
        rtype::plugins::physics::debug::debugVelocityArrow({0.0F, 0.0F}, {300.0F, 0.0F}, 0.1F, kRed, 2.0F);
    ASSERT_EQ(arrow.size(), 3U);
    for (const Shape& stroke : arrow) {
        EXPECT_EQ(stroke.kind, ShapeKind::kLine);
        EXPECT_FLOAT_EQ(stroke.radius, 1.0F);  // Half the thickness.
        EXPECT_EQ(stroke.fillColor, kRed);
    }
    EXPECT_EQ(arrow.at(0).end, (glm::vec2{30.0F, 0.0F}));  // 0.1 s of travel at 300 units/s.
    // The head strokes go back from the tip, one on each side.
    EXPECT_LT(arrow.at(1).end.x, 30.0F);
    EXPECT_LT(arrow.at(2).end.x, 30.0F);
    EXPECT_NEAR(arrow.at(1).end.y, -arrow.at(2).end.y, kTolerance);
    EXPECT_GT(std::abs(arrow.at(1).end.y), 0.0F);
}

TEST(PhysicsDebugShapes, AStillBodyHasNoArrow) {
    EXPECT_TRUE(
        rtype::plugins::physics::debug::debugVelocityArrow({5.0F, 5.0F}, {0.0F, 0.0F}, 0.1F, kRed, 1.0F).empty());
}

TEST(PhysicsDebugPlugin, RegistersItsComponentAndSettings) {
    DebugApp fixture;
    EXPECT_TRUE(fixture.components().find("rtype.PhysicsDebug"));
    EXPECT_TRUE(fixture.app.getWorld().getResources().has<PhysicsDebugSettings>());
}

TEST(PhysicsDebugPlugin, DrawsOnlyTheEntitiesWithPhysicsDebug) {
    DebugApp fixture;
    fixture.spawn({100.0F, 50.0F}, PhysicsDebug{.color = kRed});
    fixture.spawn({300.0F, 50.0F}, std::nullopt);
    const std::vector<Shape> shapes = fixture.frame();
    ASSERT_EQ(shapes.size(), 1U);
    EXPECT_EQ(shapes.at(0).kind, ShapeKind::kRect);
    EXPECT_EQ(shapes.at(0).position, (glm::vec2{100.0F, 50.0F}));
    EXPECT_EQ(shapes.at(0).outlineColor, kRed);
}

TEST(PhysicsDebugPlugin, DrawAllDrawsEveryColliderInTheSettingsColour) {
    DebugApp fixture;
    fixture.settings()->drawAll = true;
    fixture.spawn({300.0F, 50.0F}, std::nullopt);
    const std::vector<Shape> shapes = fixture.frame();
    ASSERT_EQ(shapes.size(), 1U);
    EXPECT_EQ(shapes.at(0).outlineColor, PhysicsDebugSettings{}.color);
}

TEST(PhysicsDebugPlugin, DrawsTheVelocityAsAnArrow) {
    DebugApp fixture;
    const Entity ship = fixture.spawn({0.0F, 0.0F}, PhysicsDebug{.shape = false});
    fixture.components().add(ship, Velocity{.linear = {300.0F, 0.0F}});
    const std::vector<Shape> shapes = fixture.frame();
    ASSERT_EQ(shapes.size(), 3U);  // No outline: only the arrow.
    for (const Shape& stroke : shapes) {
        EXPECT_EQ(stroke.kind, ShapeKind::kLine);
    }
}

TEST(PhysicsDebugPlugin, DrawsNothingWhenDisabled) {
    DebugApp fixture;
    fixture.settings()->enabled = false;
    fixture.spawn({100.0F, 50.0F}, PhysicsDebug{});
    EXPECT_TRUE(fixture.frame().empty());
}

TEST(PhysicsDebugPlugin, TheToggleKeyFlipsTheDrawing) {
    DebugApp fixture;
    fixture.spawn({100.0F, 50.0F}, PhysicsDebug{});
    ASSERT_EQ(fixture.frame().size(), 1U);

    fixture.state->frames.push_back(
        {rtype::plugins::backend::event::KeyPressed{.key = rtype::plugins::backend::input::Key::kF3}});
    EXPECT_TRUE(fixture.frame().empty());  // F3: off.

    fixture.state->frames.push_back(
        {rtype::plugins::backend::event::KeyPressed{.key = rtype::plugins::backend::input::Key::kF3, .repeat = true}});
    EXPECT_TRUE(fixture.frame().empty());  // An auto-repeat does not flip it back.

    fixture.state->frames.push_back(
        {rtype::plugins::backend::event::KeyPressed{.key = rtype::plugins::backend::input::Key::kF3}});
    EXPECT_EQ(fixture.frame().size(), 1U);  // F3 again: on.
}

TEST(PhysicsDebugPlugin, AddingPhysicsDebugNeverRebuildsTheBody) {
    rtype::ecs::App app;
    app.addPlugin(rtype::plugins::physics::PhysicsPlugin{rtype::plugins::physics::PhysicsSettings{
        .maxBodies = 8, .maxBodyPairs = 8, .maxContactConstraints = 8, .tempAllocatorBytes = 1024 * 1024}});
    app.addPlugin(PhysicsDebugPlugin{});
    app.getWorld().getResources().insert(rtype::ecs::Time{.fixedDelta = 1.0F / 60.0F});
    const Entity ship = app.getWorld().spawn();
    app.getWorld().getComponents().add(ship, Transform{});
    app.getWorld().getComponents().add(ship, Collider{});
    app.getWorld().getComponents().add(ship, rtype::plugins::physics::RigidBody{});
    app.getSchedules().run(rtype::ecs::Phase::kFixedUpdate);
    const rtype::plugins::physics::PhysicsHandle before =
        *app.getWorld().getComponents().get<rtype::plugins::physics::PhysicsHandle>(ship);

    app.getWorld().getComponents().add(ship, PhysicsDebug{});
    app.getSchedules().run(rtype::ecs::Phase::kFixedUpdate);
    EXPECT_EQ(*app.getWorld().getComponents().get<rtype::plugins::physics::PhysicsHandle>(ship), before);
}
