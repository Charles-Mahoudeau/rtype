/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteAnimator
*/

#pragma once

#include "plugins/AssetIdReflection.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::sprite {

/// @brief Plays an animation of an atlas on its entity: the component ("rtype.SpriteAnimator") the SpritePlugin turns
/// into the texture, source and origin of the entity's SpriteRenderer, frame after frame.
///
/// @details Only plain values, all reflected: a server can replicate which animation plays, and each process advances
/// the time on its own. To switch animation, set `animation` and put `time` back to 0. An animation that repeats a
/// finite number of times stops on its last frame: `playing` turns false and an AnimationFinished event is sent.
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {100.0F, 300.0F}})
///     .add(rtype::plugins::backend::SpriteRenderer{.layer = 5})
///     .add(rtype::plugins::sprite::SpriteAnimator{.atlas = atlases->load("assets/ship.json"),
///                                                 .animation = rtype::plugins::sprite::animationId("idle")});
/// @endcode
struct SpriteAnimator {
    core::asset::AssetId atlas;      ///< The atlas, from AtlasAssets.
    core::asset::AssetId animation;  ///< animationId() of the animation's name.
    float time = 0.0F;               ///< Seconds since the animation started, at speed 1.
    float speed = 1.0F;              ///< Multiplies the time: 2 plays twice as fast, 0 freezes it.
    bool playing = true;             ///< False: the time stands still, the current frame stays.
};
RTYPE_ECS_COMPONENT(SpriteAnimator, "rtype.SpriteAnimator", atlas, animation, time, speed, playing);

}  // namespace rtype::plugins::sprite
