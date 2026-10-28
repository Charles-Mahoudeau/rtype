/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** AnimationEvents
*/

#pragma once

#include "plugins/AssetIdReflection.hpp"
#include "rtype/ecs/entity/Entity.hpp"
#include "rtype/ecs/event/EventTraits.hpp"

namespace rtype::plugins::sprite {

/// @brief An animation that repeats a finite number of times played its last frame: sent once, by the SpritePlugin, in
/// the PostUpdate that saw it.
/// @code
/// void onExploded(rtype::ecs::EventReader<rtype::plugins::sprite::AnimationFinished> finished,
///                 rtype::ecs::Commands& commands) {
///     for (const rtype::plugins::sprite::AnimationFinished& event : finished) { commands.despawn(event.entity); }
/// }
/// @endcode
struct AnimationFinished {
    ecs::Entity entity;              ///< The entity whose SpriteAnimator played it.
    core::asset::AssetId animation;  ///< animationId() of its name.
};
RTYPE_ECS_EVENT(AnimationFinished, "rtype.AnimationFinished", entity, animation);

}  // namespace rtype::plugins::sprite
