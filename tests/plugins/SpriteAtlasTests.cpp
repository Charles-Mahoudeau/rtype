/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteAtlasTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"

using rtype::plugins::backend::math::Rect;
using rtype::plugins::sprite::AnimationDirection;
using rtype::plugins::sprite::SpriteAnimation;
using rtype::plugins::sprite::SpriteAtlas;
using rtype::plugins::sprite::exceptions::AtlasException;

namespace {
/// @return An atlas of @p count frames of 10x10 pixels, on one row.
SpriteAtlas rowOf(std::uint32_t count) { return SpriteAtlas::grid("sheet.png", {10.0F, 10.0F}, count); }

/// @return The frames an animation shows at @p times.
std::vector<std::uint32_t> framesAt(const SpriteAnimation& animation, const std::vector<float>& times) {
    std::vector<std::uint32_t> frames;
    for (const float time : times) {
        frames.push_back(animation.sample(time).frame);
    }
    return frames;
}

/// @return An animation over frames 0 to @p count - 1, each lasting 1 second.
SpriteAnimation animationOf(std::uint32_t count, AnimationDirection direction, std::uint32_t repeat = 0) {
    SpriteAnimation animation{.name = "test", .direction = direction, .repeat = repeat};
    for (std::uint32_t frame = 0; frame < count; ++frame) {
        animation.frames.push_back(frame);
        animation.durations.push_back(1.0F);
    }
    return animation;
}
}  // namespace

TEST(SpriteAtlas, NamesItsTextureByItsPath) {
    const SpriteAtlas atlas{"assets/ship.png"};
    EXPECT_EQ(atlas.getTexturePath(), "assets/ship.png");
    EXPECT_EQ(atlas.getTexture(), rtype::core::asset::AssetId::fromPath("assets/ship.png"));
}

TEST(SpriteAtlas, GridCutsRowsOfFramesWithTheirSpacing) {
    const SpriteAtlas atlas = SpriteAtlas::grid("sheet.png", {8.0F, 4.0F}, 5, {2.0F, 1.0F}, 3, {1.0F, 1.0F});
    ASSERT_EQ(atlas.getFrames().size(), 5U);
    EXPECT_EQ(atlas.getFrames().at(0).name, "0");
    EXPECT_FLOAT_EQ(atlas.getFrames().at(0).source.getX(), 1.0F);
    EXPECT_FLOAT_EQ(atlas.getFrames().at(2).source.getX(), 21.0F);  // 1 + 2 * (8 + 2)
    EXPECT_FLOAT_EQ(atlas.getFrames().at(3).source.getX(), 1.0F);   // The second row.
    EXPECT_FLOAT_EQ(atlas.getFrames().at(3).source.getY(), 6.0F);   // 1 + (4 + 1)
    EXPECT_FLOAT_EQ(atlas.getFrames().at(4).source.getWidth(), 8.0F);
}

TEST(SpriteAtlas, PutsAFrameOriginAtItsCentreByDefault) {
    SpriteAtlas atlas{"sheet.png"};
    atlas.addFrame("centred", Rect{{0.0F, 0.0F}, {16.0F, 8.0F}});
    atlas.addFrame("pivoted", Rect{{0.0F, 0.0F}, {16.0F, 8.0F}}, glm::vec2{2.0F, 3.0F});
    EXPECT_EQ(atlas.getFrames().at(0).origin, (glm::vec2{8.0F, 4.0F}));
    EXPECT_EQ(atlas.getFrames().at(1).origin, (glm::vec2{2.0F, 3.0F}));
}

TEST(SpriteAtlas, FindsAFrameByName) {
    const SpriteAtlas atlas = rowOf(3);
    EXPECT_EQ(atlas.findFrame("2"), std::optional<std::uint32_t>{2});
    EXPECT_FALSE(atlas.findFrame("missing"));
}

TEST(SpriteAtlas, RejectsTwoFramesOfTheSameName) {
    SpriteAtlas atlas = rowOf(1);
    EXPECT_THROW(atlas.addFrame("0", Rect{}), AtlasException);
}

TEST(SpriteAtlas, FindsAnAnimationByTheIdOfItsName) {
    SpriteAtlas atlas = rowOf(4);
    atlas.addAnimation("fly", {0, 1, 2}, 0.1F, AnimationDirection::kPingPong, 2);
    const SpriteAnimation* fly = atlas.findAnimation(rtype::plugins::sprite::animationId("fly"));
    ASSERT_NE(fly, nullptr);
    EXPECT_EQ(fly->frames, (std::vector<std::uint32_t>{0, 1, 2}));
    EXPECT_EQ(fly->durations, (std::vector<float>{0.1F, 0.1F, 0.1F}));
    EXPECT_EQ(fly->direction, AnimationDirection::kPingPong);
    EXPECT_EQ(fly->repeat, 2U);
    EXPECT_EQ(atlas.findAnimation(rtype::plugins::sprite::animationId("walk")), nullptr);
}

TEST(SpriteAtlas, RejectsAnAnimationOfAFrameItDoesNotHave) {
    SpriteAtlas atlas = rowOf(2);
    EXPECT_THROW(atlas.addAnimation("fly", {0, 2}, 0.1F), AtlasException);
}

TEST(SpriteAtlas, RejectsAnAnimationWithoutOneDurationPerFrame) {
    SpriteAtlas atlas = rowOf(2);
    EXPECT_THROW(atlas.addAnimation(SpriteAnimation{.name = "fly", .frames = {0, 1}, .durations = {0.1F}}),
                 AtlasException);
}

TEST(SpriteAtlas, RejectsTwoAnimationsOfTheSameName) {
    SpriteAtlas atlas = rowOf(2);
    atlas.addAnimation("fly", {0}, 0.1F);
    EXPECT_THROW(atlas.addAnimation("fly", {1}, 0.1F), AtlasException);
}

TEST(SpriteAnimation, PlaysForwardInALoop) {
    const SpriteAnimation animation = animationOf(3, AnimationDirection::kForward);
    EXPECT_FLOAT_EQ(animation.getCycleDuration(), 3.0F);
    EXPECT_EQ(framesAt(animation, {0.0F, 0.5F, 1.0F, 2.9F, 3.0F, 4.5F}),
              (std::vector<std::uint32_t>{0, 0, 1, 2, 0, 1}));
    EXPECT_FALSE(animation.sample(100.0F).finished);
}

TEST(SpriteAnimation, PlaysInReverse) {
    const SpriteAnimation animation = animationOf(3, AnimationDirection::kReverse);
    EXPECT_EQ(framesAt(animation, {0.0F, 1.0F, 2.0F, 3.0F}), (std::vector<std::uint32_t>{2, 1, 0, 2}));
}

TEST(SpriteAnimation, PingPongsWithoutShowingAnEndTwice) {
    const SpriteAnimation animation = animationOf(3, AnimationDirection::kPingPong);
    EXPECT_FLOAT_EQ(animation.getCycleDuration(), 4.0F);  // 0 1 2 1
    EXPECT_EQ(framesAt(animation, {0.0F, 1.0F, 2.0F, 3.0F, 4.0F}), (std::vector<std::uint32_t>{0, 1, 2, 1, 0}));
}

TEST(SpriteAnimation, StopsOnItsLastFrameAfterItsRepetitions) {
    const SpriteAnimation animation = animationOf(2, AnimationDirection::kForward, 2);
    EXPECT_EQ(animation.sample(3.5F).frame, 1U);
    EXPECT_FALSE(animation.sample(3.5F).finished);
    const SpriteAnimation::Sample over = animation.sample(4.0F);
    EXPECT_EQ(over.frame, 1U);
    EXPECT_TRUE(over.finished);
}

TEST(SpriteAnimation, KeepsEachFrameForItsOwnDuration) {
    const SpriteAnimation animation{.name = "blink", .frames = {0, 1}, .durations = {0.9F, 0.1F}};
    EXPECT_EQ(framesAt(animation, {0.5F, 0.95F, 1.05F}), (std::vector<std::uint32_t>{0, 1, 0}));
}

TEST(SpriteAnimation, StartsOverForANegativeTime) {
    const SpriteAnimation animation = animationOf(3, AnimationDirection::kForward);
    EXPECT_EQ(animation.sample(-1.0F).frame, 0U);
}

TEST(SpriteAnimation, ShowsFrameZeroWithoutFrames) {
    const SpriteAnimation animation{.name = "empty"};
    EXPECT_EQ(animation.sample(1.0F).frame, 0U);
    EXPECT_FALSE(animation.sample(1.0F).finished);
}
