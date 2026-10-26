/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteAtlas
*/

#pragma once

#include <cstddef>
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/graphics/Texture.hpp"
#include "plugins/backend/math/Rect.hpp"

namespace rtype::plugins::sprite {

/// @brief One image of an atlas: a part of its texture, and the point the sprite is placed and turned around.
struct AtlasFrame {
    std::string name;            ///< Unique in its atlas: "ship 0", "explosion_3", a packed file's stem...
    backend::math::Rect source;  ///< Where it is in the texture, in pixels.
    glm::vec2 origin{0.0F};      ///< Pivot, in pixels from the top-left of source (SpriteRenderer::origin).
};

/// @brief The order an animation plays its frames in, as Aseprite's tag directions.
enum class AnimationDirection : std::uint8_t {
    kForward,   ///< First to last.
    kReverse,   ///< Last to first.
    kPingPong,  ///< First to last then back, without showing either end twice in a row.
};

/// @brief A sequence of frames of an atlas, each shown for its own duration.
struct SpriteAnimation {
    /// @brief The frame an animation shows at a time.
    struct Sample {
        std::uint32_t frame = 0;  ///< Index in the atlas' frames.
        bool finished = false;    ///< The animation repeats a finite number of times and they are over.
    };

    std::string name;                   ///< Unique in its atlas; SpriteAnimator names it by animationId(name).
    std::vector<std::uint32_t> frames;  ///< Indices in the atlas' frames, in order.
    std::vector<float> durations;       ///< Seconds each frame is shown, one per frame.
    AnimationDirection direction = AnimationDirection::kForward;  ///< The order the frames are played in.
    std::uint32_t repeat = 0;                                     ///< How many times it plays; 0: forever.

    /// @return The seconds one pass takes: every frame once, or there and back for kPingPong.
    [[nodiscard]] float getCycleDuration() const noexcept;

    /// @return The frame shown @p time seconds after the animation started. Past its last repetition, the last frame
    /// it showed, with `finished` set. A negative time counts from the start.
    /// @note An animation without frames gives frame 0.
    [[nodiscard]] Sample sample(float time) const noexcept;
};

/// @brief A texture cut into named frames, and the animations made of them: what a SpriteAnimator plays.
///
/// @details Built from an Aseprite or TexturePacker JSON file (AtlasAssets::load()), from images packed at load time
/// (AtlasAssets::pack()), or in code for a sheet with no description: grid() cuts a uniform sheet, addFrame() and
/// addAnimation() describe the rest.
/// @code
/// rtype::plugins::sprite::SpriteAtlas enemy =
///     rtype::plugins::sprite::SpriteAtlas::grid("assets/r-typesheet26.gif", {63.0F, 48.0F}, 6, {2.0F, 0.0F});
/// enemy.addAnimation("fly", {0, 1, 2, 3, 4, 5}, 0.08F);
/// @endcode
class SpriteAtlas {
  public:
    /// @param texturePath The image of the atlas, loaded through TextureAssets; for a packed atlas, the name its
    /// pixels are given under.
    /// @param desc How the texture is sampled; its size is ignored.
    explicit SpriteAtlas(std::string texturePath, const backend::graphics::TextureDesc& desc = {});
    SpriteAtlas() = default;
    ~SpriteAtlas() = default;
    SpriteAtlas(const SpriteAtlas& other) = default;
    SpriteAtlas& operator=(const SpriteAtlas& other) = default;
    SpriteAtlas(SpriteAtlas&& other) noexcept = default;
    SpriteAtlas& operator=(SpriteAtlas&& other) noexcept = default;

    /// @brief Cuts a sheet of same-size frames, row by row from @p offset, named "0", "1"... with their origin at
    /// their centre.
    /// @param frameSize The size of every frame, in pixels.
    /// @param count How many frames there are.
    /// @param spacing The gap between two frames, in pixels.
    /// @param columns Frames per row; 0: all on one row.
    /// @param offset Where the first frame's top-left corner is, in pixels.
    [[nodiscard]] static SpriteAtlas grid(std::string texturePath, glm::vec2 frameSize, std::uint32_t count,
                                          glm::vec2 spacing = glm::vec2{0.0F}, std::uint32_t columns = 0,
                                          glm::vec2 offset = glm::vec2{0.0F},
                                          const backend::graphics::TextureDesc& desc = {});

    /// @return The image of the atlas, or the name of its packed pixels.
    [[nodiscard]] const std::string& getTexturePath() const noexcept { return _texturePath; }
    /// @return The id the texture has in TextureAssets: AssetId::fromPath(getTexturePath()).
    [[nodiscard]] asset::AssetId getTexture() const noexcept { return _texture; }
    /// @return How the texture is sampled.
    [[nodiscard]] const backend::graphics::TextureDesc& getTextureDesc() const noexcept { return _textureDesc; }
    /// @return The frames, in the order they were added.
    [[nodiscard]] const std::vector<AtlasFrame>& getFrames() const noexcept { return _frames; }
    /// @return The animations, by animationId() of their name.
    [[nodiscard]] const std::unordered_map<asset::AssetId, SpriteAnimation>& getAnimations() const noexcept {
        return _animations;
    }

    /// @brief Adds a frame after the others.
    /// @param origin Its pivot, in pixels from the top-left of @p source; nothing: its centre.
    /// @return This atlas, to chain calls.
    /// @throws exceptions::AtlasException If a frame already has this name.
    SpriteAtlas& addFrame(std::string name, const backend::math::Rect& source,
                          std::optional<glm::vec2> origin = std::nullopt);

    /// @brief Adds an animation whose frames all last @p frameDuration seconds.
    /// @param frames Indices in getFrames().
    /// @return This atlas, to chain calls.
    /// @throws exceptions::AtlasException If an index is not a frame, or an animation already has this name.
    SpriteAtlas& addAnimation(std::string name, std::vector<std::uint32_t> frames, float frameDuration,
                              AnimationDirection direction = AnimationDirection::kForward, std::uint32_t repeat = 0);

    /// @brief Adds an animation, each frame with its own duration.
    /// @return This atlas, to chain calls.
    /// @throws exceptions::AtlasException If an index is not a frame, the durations are not one per frame, or an
    /// animation already has this name.
    SpriteAtlas& addAnimation(SpriteAnimation animation);

    /// @return The index of the frame named @p name, or nothing.
    [[nodiscard]] std::optional<std::uint32_t> findFrame(std::string_view name) const;

    /// @return The animation @p id names (animationId() of its name), or nullptr.
    [[nodiscard]] const SpriteAnimation* findAnimation(asset::AssetId id) const;

  private:
    std::string _texturePath;                                         ///< The image, or the packed pixels' name.
    asset::AssetId _texture;                                          ///< AssetId::fromPath(_texturePath).
    backend::graphics::TextureDesc _textureDesc;                      ///< How the texture is sampled.
    std::vector<AtlasFrame> _frames;                                  ///< In the order they were added.
    std::unordered_map<std::string, std::uint32_t> _frameIndices;     ///< Index of each frame, by name.
    std::unordered_map<asset::AssetId, SpriteAnimation> _animations;  ///< By animationId() of their name.
};

/// @return The id of the animation named @p name, as SpriteAnimator::animation keeps it: the same in every process.
[[nodiscard]] asset::AssetId animationId(std::string_view name);

}  // namespace rtype::plugins::sprite
