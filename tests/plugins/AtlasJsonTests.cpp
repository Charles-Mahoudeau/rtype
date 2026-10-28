/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AtlasJsonTests
*/

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <glm/ext/vector_float2.hpp>
#include <string_view>
#include <vector>

#include "core/asset/AssetId.hpp"
#include "plugins/sprite/SpriteAtlas.hpp"
#include "plugins/sprite/detail/AtlasJson.hpp"
#include "plugins/sprite/exceptions/SpriteExceptions.hpp"

using rtype::plugins::sprite::AnimationDirection;
using rtype::plugins::sprite::animationId;
using rtype::plugins::sprite::SpriteAnimation;
using rtype::plugins::sprite::SpriteAtlas;
using rtype::plugins::sprite::detail::parseAtlasJson;
using rtype::plugins::sprite::exceptions::AtlasException;

namespace {
/// @brief What Aseprite exports ("JSON Hash"): three frames whose names do not sort in file order, and three tags.
constexpr std::string_view kAseprite = R"({
  "frames": {
    "ship 9":  { "frame": { "x": 0,  "y": 0, "w": 16, "h": 8 }, "duration": 100 },
    "ship 10": { "frame": { "x": 16, "y": 0, "w": 16, "h": 8 }, "duration": 250 },
    "ship 2":  { "frame": { "x": 32, "y": 0, "w": 16, "h": 8 } }
  },
  "meta": {
    "image": "ship.png",
    "frameTags": [
      { "name": "idle",    "from": 0, "to": 2, "direction": "pingpong" },
      { "name": "back",    "from": 0, "to": 1, "direction": "pingpong_reverse" },
      { "name": "explode", "from": 1, "to": 2, "direction": "reverse", "repeat": "1" }
    ]
  }
})";

/// @return The atlas of @p text, read from assets/sprites/ship.json.
SpriteAtlas parse(std::string_view text) { return parseAtlasJson(text, "assets/sprites/ship.json"); }

/// @return The animation @p name of @p atlas, which must have it.
const SpriteAnimation& animationOf(const SpriteAtlas& atlas, std::string_view name) {
    const SpriteAnimation* animation = atlas.findAnimation(animationId(name));
    EXPECT_NE(animation, nullptr);
    return *animation;
}
}  // namespace

TEST(AtlasJson, FindsTheImageNextToTheFile) {
    const SpriteAtlas atlas = parse(kAseprite);
    EXPECT_EQ(atlas.getTexturePath(), "assets/sprites/ship.png");
    EXPECT_EQ(atlas.getTexture(), rtype::core::asset::AssetId::fromPath("assets/sprites/ship.png"));
}

TEST(AtlasJson, KeepsTheFramesInTheFileOrder) {
    const SpriteAtlas atlas = parse(kAseprite);
    ASSERT_EQ(atlas.getFrames().size(), 3U);
    EXPECT_EQ(atlas.getFrames().at(0).name, "ship 9");
    EXPECT_EQ(atlas.getFrames().at(1).name, "ship 10");
    EXPECT_EQ(atlas.getFrames().at(2).name, "ship 2");
    EXPECT_FLOAT_EQ(atlas.getFrames().at(1).source.getX(), 16.0F);
    EXPECT_EQ(atlas.getFrames().at(1).origin, (glm::vec2{8.0F, 4.0F}));  // Centred.
}

TEST(AtlasJson, MakesAnAnimationOfEachTag) {
    const SpriteAtlas atlas = parse(kAseprite);
    const SpriteAnimation& idle = animationOf(atlas, "idle");
    EXPECT_EQ(idle.frames, (std::vector<std::uint32_t>{0, 1, 2}));
    EXPECT_EQ(idle.durations, (std::vector<float>{0.1F, 0.25F, 0.1F}));  // 100 ms when absent.
    EXPECT_EQ(idle.direction, AnimationDirection::kPingPong);
    EXPECT_EQ(idle.repeat, 0U);
}

TEST(AtlasJson, ReadsTheDirectionsAndRepeatCounts) {
    const SpriteAtlas atlas = parse(kAseprite);
    const SpriteAnimation& back = animationOf(atlas, "back");
    EXPECT_EQ(back.direction, AnimationDirection::kPingPong);
    EXPECT_EQ(back.frames, (std::vector<std::uint32_t>{1, 0}));  // pingpong_reverse starts from the end.
    const SpriteAnimation& explode = animationOf(atlas, "explode");
    EXPECT_EQ(explode.direction, AnimationDirection::kReverse);
    EXPECT_EQ(explode.repeat, 1U);
}

TEST(AtlasJson, ReadsTheArrayLayoutWithPivotsAndTrimming) {
    // TexturePacker's "JSON Array": a 20x20 frame trimmed to 10x10 from (4, 6), pivot at its bottom centre.
    const SpriteAtlas atlas = parse(R"({
      "frames": [
        { "filename": "coin.png", "frame": { "x": 2, "y": 3, "w": 10, "h": 10 }, "trimmed": true,
          "spriteSourceSize": { "x": 4, "y": 6, "w": 10, "h": 10 }, "sourceSize": { "w": 20, "h": 20 },
          "pivot": { "x": 0.5, "y": 1.0 } }
      ],
      "meta": { "image": "coins.png" }
    })");
    ASSERT_EQ(atlas.getFrames().size(), 1U);
    EXPECT_EQ(atlas.getFrames().at(0).name, "coin.png");
    EXPECT_EQ(atlas.getFrames().at(0).origin, (glm::vec2{6.0F, 14.0F}));  // (10, 20) in the untrimmed frame.
    EXPECT_TRUE(atlas.getAnimations().empty());
}

TEST(AtlasJson, RejectsTextThatIsNotJson) { EXPECT_THROW(static_cast<void>(parse("{ not json")), AtlasException); }

TEST(AtlasJson, RejectsAFileWithoutItsImage) {
    EXPECT_THROW(static_cast<void>(parse(R"({ "frames": {}, "meta": {} })")), AtlasException);
}

TEST(AtlasJson, RejectsATagPastTheLastFrame) {
    EXPECT_THROW(static_cast<void>(parse(R"({
      "frames": { "a": { "frame": { "x": 0, "y": 0, "w": 1, "h": 1 } } },
      "meta": { "image": "a.png", "frameTags": [ { "name": "t", "from": 0, "to": 3 } ] }
    })")),
                 AtlasException);
}

TEST(AtlasJson, RejectsAnUnknownDirection) {
    EXPECT_THROW(static_cast<void>(parse(R"({
      "frames": { "a": { "frame": { "x": 0, "y": 0, "w": 1, "h": 1 } } },
      "meta": { "image": "a.png", "frameTags": [ { "name": "t", "from": 0, "to": 0, "direction": "sideways" } ] }
    })")),
                 AtlasException);
}
