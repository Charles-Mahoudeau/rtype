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
