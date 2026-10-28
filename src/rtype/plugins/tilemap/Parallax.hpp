/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** Parallax
*/

#pragma once

#include <glm/ext/vector_float2.hpp>

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::tilemap {

/// @brief Moves an entity slower or faster than the camera, and scrolls it on its own: the component "rtype.Parallax",
/// for backgrounds and foregrounds.
///
/// @details The TilemapPlugin's Render system "rtype.Tilemap.Parallax" (client only) sets the entity's GlobalTransform
/// to its Transform, moved by `(camera - origin) * (1 - factor)` and by what it scrolled, `scroll` per second, so far.
/// The camera is the GlobalTransform of the Camera entity with the highest priority. Works on any entity: a tilemap
/// layer (spawnTilemap() gives each one the parallax factor and scroll Tiled gives it) or a sprite.
/// @code
/// // A far background: moves at a tenth of the camera's speed, and drifts left on its own.
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{})
///     .add(rtype::plugins::backend::SpriteRenderer{.texture = sky, .layer = -100})
///     .add(rtype::plugins::tilemap::Parallax{.factor = {0.1F, 0.1F}, .scroll = {-8.0F, 0.0F}});
/// @endcode
struct Parallax {
    glm::vec2 factor{1.0F};    ///< 0: stays on screen with the camera; 1: moves with the world; above 1: a foreground.
    glm::vec2 scroll{0.0F};    ///< World units per second it moves on its own.
    glm::vec2 origin{0.0F};    ///< Where the camera sees the entity at its Transform, in world units.
    glm::vec2 scrolled{0.0F};  ///< How far it scrolled so far: scroll accumulated over time.
};
RTYPE_ECS_COMPONENT(Parallax, "rtype.Parallax", factor, scroll, origin, scrolled);

}  // namespace rtype::plugins::tilemap
