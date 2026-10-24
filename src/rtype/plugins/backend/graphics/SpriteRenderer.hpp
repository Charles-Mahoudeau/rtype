/*
** EPITECH PROJECT, 2026
** rtype
** File description:
** SpriteRenderer
*/

#pragma once

#include <cstdint>
#include <glm/ext/vector_float2.hpp>

#include "plugins/GlmReflection.hpp"  // IWYU pragma: keep
#include "plugins/asset/AssetId.hpp"
#include "plugins/backend/graphics/Color.hpp"
#include "plugins/backend/math/Rect.hpp"
#include "rtype/ecs/component/ComponentTraits.hpp"

namespace rtype::plugins::backend {
/// @brief Draws an entity as a part of a texture: the component a render system will turn into a graphics::Sprite.
///
/// @details Only what to draw: where (position, scale, rotation) comes from the entity's GlobalTransform. The texture
/// is referred to by its AssetId, the same in every process, which TextureAssets::load() returns at once; the
/// BackendPlugin's Render system draws the entity once the texture is loaded, sorted by layer. Every field but `source`
/// is reflected (Luau, network).
/// @code
/// commands.spawn()
///     .add(rtype::plugins::transform::Transform{.position = {100.0F, 300.0F}})
///     .add(rtype::plugins::backend::SpriteRenderer{
///         .texture = textures->load("assets/ship.png"), .source = {{0.0F, 0.0F}, {32.0F, 16.0F}}, .layer = 10});
/// @endcode
struct SpriteRenderer {
    asset::AssetId texture;       ///< The texture to draw, from TextureAssets::load().
    math::Rect source;            ///< Part of the texture to show, in pixels (a frame of a sprite sheet).
    graphics::Color color{1.0F};  ///< Multiplies the texture: tint and transparency.
    glm::vec2 origin{0.0F};       ///< Pivot of the position and the rotation, in pixels from the top-left of source.
    std::int32_t layer{0};        ///< Draw order: higher layers are drawn on top.
};
RTYPE_ECS_COMPONENT(SpriteRenderer, "rtype.SpriteRenderer", texture, color, origin, layer);
}  // namespace rtype::plugins::backend
