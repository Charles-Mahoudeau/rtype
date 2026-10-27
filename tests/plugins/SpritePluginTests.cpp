/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpritePluginTests
*/

#include <gtest/gtest.h>

#include <glm/ext/vector_float2.hpp>

#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/graphics/SpriteRenderer.hpp"
#include "plugins/backend/graphics/TextureAssets.hpp"
#include "plugins/sprite/AnimationEvents.hpp"
#include "plugins/sprite/AtlasAssets.hpp"
#include "plugins/sprite/SpriteAnimator.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/SpritePlugin.hpp"
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventCursor.hpp"
#include "rtype/ecs/schedule/Phase.hpp"
#include "rtype/ecs/time/Time.hpp"
#include "rtype/ecs/type/Field.hpp"
#include "rtype/ecs/world/World.hpp"

using rtype::plugins::asset::AssetId;
using rtype::plugins::backend::SpriteRenderer;
using rtype::plugins::sprite::AnimationDirection;
using rtype::plugins::sprite::animationId;
using rtype::plugins::sprite::SpriteAnimator;
using rtype::plugins::sprite::SpriteAtlas;

namespace {
/// @brief An App with the SpritePlugin, an atlas of four 10x10 frames, and frames of 0.25 seconds.
class SpriteApp {
  public:
    SpriteApp() {
        _app.addPlugin(rtype::plugins::sprite::SpritePlugin{});
        _app.getWorld().getResources().insert(rtype::ecs::Time{.delta = 0.25F});
        SpriteAtlas atlas = SpriteAtlas::grid("sheet.png", {10.0F, 10.0F}, 4);
        atlas.addAnimation("loop", {0, 1, 2, 3}, 0.5F);
        atlas.addAnimation("once", {2, 3}, 0.5F, AnimationDirection::kForward, 1);
        _atlas = _app.getWorld().getResources().getMut<rtype::plugins::sprite::AtlasAssets>()->add("sheet", atlas);
    }
    ~SpriteApp() = default;
    SpriteApp(const SpriteApp& other) = delete;
    SpriteApp& operator=(const SpriteApp& other) = delete;
    SpriteApp(SpriteApp&& other) = delete;
    SpriteApp& operator=(SpriteApp&& other) = delete;

    [[nodiscard]] rtype::ecs::World& world() noexcept { return _app.getWorld(); }
    [[nodiscard]] AssetId atlas() const noexcept { return _atlas; }

    /// @return A new entity playing @p animation, with a SpriteRenderer.
    rtype::ecs::Entity spawn(const char* animation) {
        const rtype::ecs::Entity entity = world().spawn();
        world().getComponents().add(entity, SpriteRenderer{});
        world().getComponents().add(entity, SpriteAnimator{.atlas = _atlas, .animation = animationId(animation)});
        return entity;
    }

    /// @brief Runs PostUpdate, where the plugin's systems are.
    void postUpdate() { _app.getSchedules().run(rtype::ecs::Phase::kPostUpdate); }

  private:
    rtype::ecs::App _app;  ///< The App under test.
    AssetId _atlas;        ///< The test atlas.
};
}  // namespace

TEST(SpriteAnimator, ReflectsEveryField) {
    using Traits = rtype::ecs::ComponentTraits<SpriteAnimator>;
    EXPECT_EQ(Traits::kName, "rtype.SpriteAnimator");
    ASSERT_EQ(Traits::kFields.size(), 5U);
    EXPECT_EQ(Traits::kFields[0].type, rtype::ecs::FieldType::kAssetId);
    EXPECT_EQ(Traits::kFields[1].type, rtype::ecs::FieldType::kAssetId);
    EXPECT_EQ(Traits::kFields[4].name, "playing");
}

TEST(SpritePlugin, InsertsTheAtlasAndTextureAssets) {
    SpriteApp app;
    EXPECT_TRUE(app.world().getResources().has<rtype::plugins::sprite::AtlasAssets>());
    EXPECT_TRUE(app.world().getResources().has<rtype::plugins::backend::TextureAssets>());
}

TEST(SpritePlugin, ShowsTheFrameOfTheAnimationTime) {
    SpriteApp app;
    const rtype::ecs::Entity entity = app.spawn("loop");
    app.postUpdate();  // 0.25 s: frame 0.
    app.postUpdate();  // 0.5 s: frame 1.
    const SpriteRenderer* renderer = app.world().getComponents().get<SpriteRenderer>(entity);
    ASSERT_NE(renderer, nullptr);
    EXPECT_EQ(renderer->texture, AssetId::fromPath("sheet.png"));
    EXPECT_FLOAT_EQ(renderer->source.getX(), 10.0F);
    EXPECT_EQ(renderer->origin, (glm::vec2{5.0F, 5.0F}));
    EXPECT_FLOAT_EQ(app.world().getComponents().get<SpriteAnimator>(entity)->time, 0.5F);
}

TEST(SpritePlugin, PlaysAtTheAnimatorSpeedAndPauses) {
    SpriteApp app;
    const rtype::ecs::Entity entity = app.spawn("loop");
    app.world().getComponents().getMut<SpriteAnimator>(entity)->speed = 2.0F;
    app.postUpdate();
    EXPECT_FLOAT_EQ(app.world().getComponents().get<SpriteAnimator>(entity)->time, 0.5F);
    app.world().getComponents().getMut<SpriteAnimator>(entity)->playing = false;
    app.postUpdate();
    EXPECT_FLOAT_EQ(app.world().getComponents().get<SpriteAnimator>(entity)->time, 0.5F);
}

TEST(SpritePlugin, StopsAFiniteAnimationAndSaysSo) {
    SpriteApp app;
    const rtype::ecs::Entity entity = app.spawn("once");
    for (int frame = 0; frame < 5; ++frame) {
        app.postUpdate();
    }
    const SpriteAnimator* animator = app.world().getComponents().get<SpriteAnimator>(entity);
    EXPECT_FALSE(animator->playing);
    EXPECT_FLOAT_EQ(app.world().getComponents().get<SpriteRenderer>(entity)->source.getX(), 30.0F);  // Frame 3.
    rtype::ecs::EventCursor cursor;
    const auto finished = app.world().getEvents().read<rtype::plugins::sprite::AnimationFinished>(cursor);
    ASSERT_EQ(finished.size(), 1U);
    const rtype::plugins::sprite::AnimationFinished& event = *finished.begin();
    EXPECT_EQ(event.entity, entity);
    EXPECT_EQ(event.animation, animationId("once"));
}

TEST(SpritePlugin, SkipsAnUnknownAnimation) {
    SpriteApp app;
    const rtype::ecs::Entity entity = app.spawn("missing");
    app.postUpdate();
    EXPECT_FLOAT_EQ(app.world().getComponents().get<SpriteAnimator>(entity)->time, 0.0F);
    EXPECT_EQ(app.world().getComponents().get<SpriteRenderer>(entity)->texture, AssetId{});
}

TEST(SpritePlugin, HandsTheAtlasTexturesToTextureAssets) {
    SpriteApp app;
    app.postUpdate();
    const auto* entry =
        app.world().getResources().get<rtype::plugins::backend::TextureAssets>()->get(AssetId::fromPath("sheet.png"));
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->path, "sheet.png");
}
