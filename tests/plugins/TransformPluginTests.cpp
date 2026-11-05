/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** TransformPluginTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "plugins/transform/Transform.hpp"
#include "plugins/transform/TransformPlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/hierarchy/Parent.hpp"
#include "rtype/ecs/schedule/RunMode.hpp"
#include "rtype/ecs/type/Field.hpp"

namespace {
using rtype::plugins::transform::GlobalTransform;
using rtype::plugins::transform::Transform;

/// @brief An App with the TransformPlugin.
struct TransformApp {
    rtype::ecs::App app;  ///< The App under test.

    TransformApp() { app.addPlugin(rtype::plugins::transform::TransformPlugin{}); }

    /// @return The entity's GlobalTransform, or nullptr.
    [[nodiscard]] const GlobalTransform* globalOf(rtype::ecs::Entity entity) {
        return app.getWorld().getComponents().get<GlobalTransform>(entity);
    }

    /// @return A new entity at @p transform.
    rtype::ecs::Entity spawn(const Transform& transform) {
        const rtype::ecs::Entity entity = app.getWorld().spawn();
        app.getWorld().getComponents().add(entity, transform);
        return entity;
    }

    /// @return A new entity at @p transform, child of @p parent.
    rtype::ecs::Entity spawnChild(rtype::ecs::Entity parent, const Transform& transform) {
        const rtype::ecs::Entity entity = spawn(transform);
        EXPECT_TRUE(app.getWorld().setParent(entity, parent));
        return entity;
    }

    /// @brief Moves @p entity to @p position, as gameplay does.
    void move(rtype::ecs::Entity entity, glm::vec2 position) {
        app.getWorld().getComponents().getMut<Transform>(entity)->position = position;
    }

    /// @return Where @p entity is in the world.
    [[nodiscard]] glm::vec2 positionOf(rtype::ecs::Entity entity) {
        const GlobalTransform* global = globalOf(entity);
        EXPECT_NE(global, nullptr);
        return global != nullptr ? global->position : glm::vec2{};
    }
};
}  // namespace

TEST(Transform, ReflectsItsFields) {
    using Traits = rtype::ecs::ComponentTraits<Transform>;
    EXPECT_EQ(Traits::kName, "rtype.Transform");
    ASSERT_EQ(Traits::kFields.size(), 3U);
    EXPECT_EQ(Traits::kFields[0].type, rtype::ecs::FieldType::kVec2);
    EXPECT_EQ(Traits::kFields[1].type, rtype::ecs::FieldType::kF32);
    EXPECT_EQ(Traits::kFields[2].type, rtype::ecs::FieldType::kVec2);
    EXPECT_EQ(rtype::ecs::ComponentTraits<GlobalTransform>::kName, "rtype.GlobalTransform");
}

TEST(TransformPlugin, RegistersTheComponents) {
    TransformApp fixture;
    EXPECT_TRUE(fixture.app.getWorld().getComponents().find("rtype.Transform"));
    EXPECT_TRUE(fixture.app.getWorld().getComponents().find("rtype.GlobalTransform"));
}

TEST(TransformPlugin, GivesAGlobalTransformEqualToTheTransform) {
    TransformApp fixture;
    const rtype::ecs::Entity ship = fixture.app.getWorld().spawn();
    fixture.app.getWorld().getComponents().add(
        ship, Transform{.position = {10.0F, 20.0F}, .rotation = 0.5F, .scale = {2.0F, 3.0F}});

    fixture.app.runFrame();

    const GlobalTransform* global = fixture.globalOf(ship);
    ASSERT_NE(global, nullptr);
    EXPECT_EQ(global->position, (glm::vec2{10.0F, 20.0F}));
    EXPECT_FLOAT_EQ(global->rotation, 0.5F);
    EXPECT_EQ(global->scale, (glm::vec2{2.0F, 3.0F}));
}

TEST(TransformPlugin, FollowsTheTransformWhenItChanges) {
    TransformApp fixture;
    const rtype::ecs::Entity ship = fixture.app.getWorld().spawn();
    fixture.app.getWorld().getComponents().add(ship, Transform{.position = {10.0F, 20.0F}});
    fixture.app.runFrame();

    fixture.app.getWorld().getComponents().getMut<Transform>(ship)->position = {30.0F, 40.0F};
    fixture.app.runFrame();

    ASSERT_NE(fixture.globalOf(ship), nullptr);
    EXPECT_EQ(fixture.globalOf(ship)->position, (glm::vec2{30.0F, 40.0F}));
}

TEST(TransformPlugin, RunsOnEverySide) {
    for (const rtype::ecs::RunMode mode : {rtype::ecs::RunMode::kClient, rtype::ecs::RunMode::kServer}) {
        rtype::ecs::App app{mode};
        app.addPlugin(rtype::plugins::transform::TransformPlugin{});
        const rtype::ecs::Entity ship = app.getWorld().spawn();
        app.getWorld().getComponents().add(ship, Transform{.position = {1.0F, 2.0F}});
        app.runFrame();
        EXPECT_NE(app.getWorld().getComponents().get<GlobalTransform>(ship), nullptr);
    }
}

TEST(TransformPlugin, LeavesEntitiesWithoutATransformAlone) {
    TransformApp fixture;
    const rtype::ecs::Entity marker = fixture.app.getWorld().spawn();
    fixture.app.runFrame();
    EXPECT_EQ(fixture.globalOf(marker), nullptr);
}

TEST(TransformHierarchy, PlacesAChildOnItsFirstFrame) {
    TransformApp fixture;
    const rtype::ecs::Entity parent = fixture.spawn(Transform{.position = {100.0F, 50.0F}});
    const rtype::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {10.0F, 5.0F}});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{110.0F, 55.0F}));
}

TEST(TransformHierarchy, PlacesAChainSpawnedInOneFrameWhateverTheOrder) {
    TransformApp fixture;
    const rtype::ecs::Entity leaf = fixture.spawn(Transform{.position = {1.0F, 0.0F}});
    const rtype::ecs::Entity middle = fixture.spawn(Transform{.position = {10.0F, 0.0F}});
    const rtype::ecs::Entity root = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    EXPECT_TRUE(fixture.app.getWorld().setParent(leaf, middle));
    EXPECT_TRUE(fixture.app.getWorld().setParent(middle, root));
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(leaf), (glm::vec2{111.0F, 0.0F}));
}

TEST(TransformHierarchy, PlacesANewChildOfAnEntityAlreadyPlaced) {
    TransformApp fixture;
    const rtype::ecs::Entity parent = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    fixture.app.runFrame();
    const rtype::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {10.0F, 0.0F}});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{110.0F, 0.0F}));
}

TEST(TransformHierarchy, MovesTheWholeSubtreeWithItsRoot) {
    TransformApp fixture;
    const rtype::ecs::Entity root = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    const rtype::ecs::Entity child = fixture.spawnChild(root, Transform{.position = {10.0F, 0.0F}});
    const rtype::ecs::Entity leaf = fixture.spawnChild(child, Transform{.position = {1.0F, 0.0F}});
    fixture.app.runFrame();
    fixture.move(root, {200.0F, 50.0F});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{210.0F, 50.0F}));
    EXPECT_EQ(fixture.positionOf(leaf), (glm::vec2{211.0F, 50.0F}));
}

TEST(TransformHierarchy, FollowsAChangeOfParent) {
    TransformApp fixture;
    const rtype::ecs::Entity first = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    const rtype::ecs::Entity second = fixture.spawn(Transform{.position = {500.0F, 0.0F}});
    const rtype::ecs::Entity child = fixture.spawnChild(first, Transform{.position = {10.0F, 0.0F}});
    fixture.app.runFrame();
    EXPECT_TRUE(fixture.app.getWorld().setParent(child, second));
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{510.0F, 0.0F}));
}

TEST(TransformHierarchy, TreatsADetachedChildAsARoot) {
    TransformApp fixture;
    const rtype::ecs::Entity parent = fixture.spawn(Transform{.position = {100.0F, 0.0F}});
    const rtype::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {10.0F, 0.0F}});
    fixture.app.runFrame();
    EXPECT_TRUE(fixture.app.getWorld().removeParent(child));
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{10.0F, 0.0F}));
}

TEST(TransformHierarchy, TreatsAChildOfAnEntityWithoutTransformAsARoot) {
    TransformApp fixture;
    const rtype::ecs::Entity parent = fixture.app.getWorld().spawn();
    const rtype::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {10.0F, 0.0F}});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{10.0F, 0.0F}));
    fixture.move(child, {20.0F, 0.0F});
    fixture.app.runFrame();
    EXPECT_EQ(fixture.positionOf(child), (glm::vec2{20.0F, 0.0F}));
}

TEST(TransformHierarchy, CombinesRotationAndScale) {
    TransformApp fixture;
    const rtype::ecs::Entity parent = fixture.spawn(Transform{.scale = {2.0F, 2.0F}});
    const rtype::ecs::Entity child =
        fixture.spawnChild(parent, Transform{.position = {10.0F, 0.0F}, .rotation = 0.5F, .scale = {3.0F, 3.0F}});
    fixture.app.runFrame();
    const GlobalTransform* global = fixture.globalOf(child);
    ASSERT_NE(global, nullptr);
    EXPECT_EQ(global->position, (glm::vec2{20.0F, 0.0F}));
    EXPECT_FLOAT_EQ(global->rotation, 0.5F);
    EXPECT_EQ(global->scale, (glm::vec2{6.0F, 6.0F}));
}

TEST(TransformHierarchy, DoesNotMarkAnEntityThatDidNotMoveAsChanged) {
    TransformApp fixture;
    const rtype::ecs::Entity plain = fixture.spawn(Transform{.position = {1.0F, 0.0F}});
    const rtype::ecs::Entity parent = fixture.spawn(Transform{.position = {2.0F, 0.0F}});
    const rtype::ecs::Entity child = fixture.spawnChild(parent, Transform{.position = {3.0F, 0.0F}});
    fixture.app.runFrame();
    fixture.app.runFrame();
    const rtype::ecs::Components& components = fixture.app.getWorld().getComponents();
    const auto plainTicks = components.getTicks<GlobalTransform>(plain);
    const auto childTicks = components.getTicks<GlobalTransform>(child);
    ASSERT_TRUE(plainTicks && childTicks);
    fixture.app.runFrame();
    EXPECT_EQ(components.getTicks<GlobalTransform>(plain)->changed, plainTicks->changed);
    EXPECT_EQ(components.getTicks<GlobalTransform>(child)->changed, childTicks->changed);
}
