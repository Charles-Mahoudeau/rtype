/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteCalls
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <limits>
#include <string_view>

#include "core/asset/AssetId.hpp"
#include "plugins/AssetIdReflection.hpp"  // IWYU pragma: keep
#include "plugins/GlmReflection.hpp"      // IWYU pragma: keep
#include "rtype/ecs/app/App.hpp"
#include "rtype/ecs/call/CallTraits.hpp"
#include "rtype/ecs/type/String.hpp"

namespace rtype::plugins::sprite {

inline constexpr std::string_view kLoadAtlasCall = "rtype.sprite.loadAtlas";        ///< AtlasAssets::load().
inline constexpr std::string_view kCreateAtlasCall = "rtype.sprite.createAtlas";    ///< AtlasAssets::add().
inline constexpr std::string_view kAddFrameCall = "rtype.sprite.addFrame";          ///< SpriteAtlas::addFrame().
inline constexpr std::string_view kAddAnimationCall = "rtype.sprite.addAnimation";  ///< SpriteAtlas::addAnimation().

/// @brief The arguments of `rtype.sprite.loadAtlas`: reads an Aseprite or TexturePacker JSON file.
struct LoadAtlasArguments {
    ecs::String path;    ///< The JSON file.
    bool linear{false};  ///< Smooth sampling instead of sharp pixels.
};
RTYPE_ECS_CALL_DATA(LoadAtlasArguments, "rtype.sprite.LoadAtlasArguments", path, linear);

/// @brief The arguments of `rtype.sprite.createAtlas`: an atlas made in code, cut as a grid (SpriteAtlas::grid())
/// when `count` is not 0, empty otherwise, for `rtype.sprite.addFrame` to fill.
struct CreateAtlasArguments {
    ecs::String name;           ///< What the atlas is known by; empty: its texture's path.
    ecs::String texture;        ///< The image of the atlas.
    glm::vec2 frameSize{0.0F};  ///< The grid: the size of every frame, in pixels.
    std::uint32_t count{0};     ///< The grid: how many frames there are; 0: no grid.
    glm::vec2 spacing{0.0F};    ///< The grid: the gap between two frames, in pixels.
    std::uint32_t columns{0};   ///< The grid: frames per row; 0: all on one row.
    glm::vec2 offset{0.0F};     ///< The grid: where the first frame's top-left corner is, in pixels.
    bool linear{false};         ///< Smooth sampling instead of sharp pixels.
};
RTYPE_ECS_CALL_DATA(CreateAtlasArguments, "rtype.sprite.CreateAtlasArguments", name, texture, frameSize, count, spacing,
                    columns, offset, linear);

/// @brief The result of `rtype.sprite.loadAtlas` and `rtype.sprite.createAtlas`.
struct AtlasResult {
    core::asset::AssetId atlas;  ///< The atlas's id, for a SpriteAnimator and the calls below.
};
RTYPE_ECS_CALL_DATA(AtlasResult, "rtype.sprite.AtlasResult", atlas);

/// @brief The arguments of `rtype.sprite.addFrame`: adds a frame after the others.
struct AddFrameArguments {
    core::asset::AssetId atlas;  ///< The atlas, from `rtype.sprite.createAtlas` (or its name).
    ecs::String name;            ///< The frame's name; empty: its index ("0", "1"...), as grid() names them.
    glm::vec2 position{0.0F};    ///< The top-left corner of the frame in the texture, in pixels.
    glm::vec2 size{0.0F};        ///< Its size, in pixels.
    /// @brief Its pivot, in pixels from its top-left corner; NaN (not given): its centre.
    glm::vec2 origin{std::numeric_limits<float>::quiet_NaN()};
};
RTYPE_ECS_CALL_DATA(AddFrameArguments, "rtype.sprite.AddFrameArguments", atlas, name, position, size, origin);

/// @brief The result of `rtype.sprite.addFrame`.
struct FrameResult {
    std::uint32_t frame{0};  ///< The frame's index in the atlas.
};
RTYPE_ECS_CALL_DATA(FrameResult, "rtype.sprite.FrameResult", frame);

/// @brief The arguments of `rtype.sprite.addAnimation`: an animation of the frames `first` to `first + count - 1`.
struct AddAnimationArguments {
    core::asset::AssetId atlas;        ///< The atlas, from `rtype.sprite.createAtlas` (or its name).
    ecs::String name;                  ///< The animation's name, as a SpriteAnimator names it (animationId()).
    std::uint32_t first{0};            ///< The index of its first frame.
    std::uint32_t count{0};            ///< How many frames it has; 0: up to the atlas's last frame.
    float frameDuration{0.1F};         ///< Seconds per frame.
    ecs::String direction{"forward"};  ///< "forward", "reverse" or "pingPong".
    std::uint32_t plays{0};            ///< How many times it plays; 0: forever (`repeat` is a Luau keyword).
};
RTYPE_ECS_CALL_DATA(AddAnimationArguments, "rtype.sprite.AddAnimationArguments", atlas, name, first, count,
                    frameDuration, direction, plays);

/// @brief Registers the sprite engine calls on @p app, for scripts: `rtype.sprite.loadAtlas`,
/// `rtype.sprite.createAtlas`, `rtype.sprite.addFrame` and `rtype.sprite.addAnimation`, each on the
/// `rtype.AtlasAssets` resource. They throw an AtlasException naming what is wrong (an unknown atlas or direction, an
/// index that is not a frame, a name already taken). Called by the SpritePlugin.
void addSpriteCalls(ecs::App& app);

}  // namespace rtype::plugins::sprite
